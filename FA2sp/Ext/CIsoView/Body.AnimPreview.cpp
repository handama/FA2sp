#include "Body.h"
#include "DirectXCore.h"
#include "../CLoading/Body.h"
#include "../../Miscs/Palettes.h"
#include "../../Helpers/FString.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/TheaterHelpers.h"
#include "../../ExtraWindow/Common.h"

#include <CINI.h>
#include <CMixFile.h>
#include <CShpFile.h>
#include <CPalette.h>
#include <CMapData.h>
#include <CFinalSunDlg.h>
#include <FAMemory.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

// ============================================================================
// Animation preview playback.
//
// Overlays a multi-frame SHP animation on top of the already rendered iso view:
//   * Frame sequence and rate follow art.ini Start/End/LoopStart/LoopEnd/
//     LoopCount/Rate (see https://modenc.renegadeprojects.com/Rate and
//     https://modenc.renegadeprojects.com/Animation_Looping).
//   * Playback is driven by a window timer on the main dialog; frames advance in
//     CFinalSunDlgExt::PreTranslateMessageExt (see Ext/CFinalSunDlg/Body.cpp).
//   * DirectDraw: before every frame the visible area is restored from the game's
//     TempBuffer - the only copy of the clean canvas that the game's own painting
//     code never overwrites - the frame is drawn on top of it, and the result is
//     presented exactly like the game's mouse-move flow does.
//   * DirectX: drawn in screen space, erased by the core's background cache.
//   * Any SpecialDraw / SpecialDrawDirectX call (mouse move, canvas redraw)
//     stops playback immediately.
// ============================================================================

namespace AnimPreview
{
namespace
{
    // Timer id on the main dialog (kept clear of existing ids).
    constexpr UINT_PTR TIMER_ID = 0xA701;

    // Rate base: 900 ticks per minute => 15 ticks/second (in-game speed).
    constexpr int RATE_BASE = 900;
    // realtime: 1s = 2 game seconds
    constexpr int TICKS_PER_SECOND = 30;

    constexpr int MIN_INTERVAL_MS = 15;
    constexpr int MAX_INTERVAL_MS = 60000;

    // Must match the offsets applied inside CIsoViewExt::BlitSHPTransparent.
    constexpr int BLIT_X_OFFSET = 31;
    constexpr int BLIT_Y_OFFSET = -29;
    // Matches map objects so the image sits in the middle of the cell.
    constexpr int ANCHOR_Y_OFFSET = 15;

    // Source id used for the Report sound in SoundPlayer (0/1/2 belong to jumps).
    constexpr int REPORT_SOUND_SOURCE = 3;

    struct AnimFrames
    {
        // Frames of every shape of the Next chain, concatenated in playback order.
        std::vector<std::unique_ptr<ImageDataClassSafe>> Frames;
        // Frames pre-scaled by 1/ScaledFactor: the DirectDraw path copies raw
        // bitmaps and cannot scale while blitting.
        std::vector<std::unique_ptr<ImageDataClassSafe>> ScaledFrames;
        double ScaledInv = -1.0;

        // Expanded playback sequence (entries are indices into Frames).
        std::vector<int> Sequence;
        // Display time of every Sequence entry, taken from its own Rate.
        std::vector<int> Delays;
        // Draw alpha of every Sequence entry, taken from its own Translucency.
        std::vector<unsigned char> Alphas;
        // Index in Sequence where the endless repetition starts (infinite loops only).
        int LoopBegin = 0;
        // LoopCount < 0 in art.ini (infinite).
        bool Infinite = false;

        // First frame of every shape of the chain, and of every iteration of it,
        // paired with the Report= sound list id of the shape that owns that frame.
        std::vector<std::pair<int, FString>> ReportPoints;
    };

    bool g_Playing = false;
    AnimFrames* g_Current = nullptr;
    FString g_PlayingId;
    MapCoord g_Anchor{ 0, 0 };
    int g_Pos = 0;

    // Frame cache. Kept alive forever: the DirectX texture cache is keyed by the
    // image object address, so a recycled address would return a stale texture.
    std::vector<std::unique_ptr<AnimFrames>> g_AllFrames;
    AnimFrames* g_Cached = nullptr;
    FString g_CachedId;

    // Report sound started by this playback. Only used to decide whether an
    // interruption should stop it, so a sound started by other code survives.
    bool g_ReportStarted = false;
    FString g_ReportName;

    // DirectDraw: the previous frame is erased by restoring the visible area from
    // the game's TempBuffer (see RestoreCleanRegion), so no private copy is kept.

    // ---------------------------------------------------------------------
    // Resource resolution
    // ---------------------------------------------------------------------

    struct AnimChain
    {
        // Shapes in playback order.
        std::vector<FString> Ids;
        // Endless playback: -1 means the chain plays once and stops, otherwise it
        // is the chain entry the endless part restarts from.
        int LoopFrom = -1;
        // True when the endless part is defined by the shape itself (LoopCount < 0),
        // in which case its own LoopStart/LoopEnd give the loop point.
        bool OwnLoopPoint = false;
    };

    // Collects the whole art.ini Next chain in playback order. Following the chain
    // stops when Next is missing (play once), at an endless shape (LoopCount < 0)
    // or when the chain closes on itself (A -> Next=B -> Next=A keeps looping).
    AnimChain ResolveAnimChain(const FString& image)
    {
        AnimChain chain;
        FString cur = image;

        for (int guard = 0; guard < 64; ++guard)
        {
            // A shape that is already in the chain closes the loop: replay from its
            // first occurrence forever.
            const auto existing = std::find(chain.Ids.begin(), chain.Ids.end(), cur);
            if (existing != chain.Ids.end())
            {
                chain.LoopFrom = static_cast<int>(existing - chain.Ids.begin());
                break;
            }

            chain.Ids.push_back(cur);

            // LoopCount < 0 never advances to Next, it loops inside the shape.
            if (CINI::Art->GetInteger(cur, "LoopCount", 1) < 0)
            {
                chain.LoopFrom = static_cast<int>(chain.Ids.size()) - 1;
                chain.OwnLoopPoint = true;
                break;
            }

            auto next = CINI::Art->TryGetString(cur, "Next");
            if (!next || next->IsEmpty())
                break;

            cur = *next;
        }

        return chain;
    }

    // Rate: n = trunc(900 / Rate) => one frame per n logic ticks. Rate == 0 or
    // Rate > 900 means n == 0, which freezes the shape (a long delay is used).
    int GetFrameDelayMs(const FString& artId)
    {
        const int rate = CINI::Art->GetInteger(artId, "Rate", RATE_BASE);
        const int n = (rate > 0) ? (RATE_BASE / rate) : 0;
        if (n <= 0)
            return MAX_INTERVAL_MS;

        return std::clamp(n * 1000 / TICKS_PER_SECOND, MIN_INTERVAL_MS, MAX_INTERVAL_MS);
    }

    // art.ini Translucency is a transparency percentage, so the draw alpha is its
    // complement (same formula as CLoadingExt::LoadObjects).
    unsigned char GetShapeAlpha(const FString& artId)
    {
        const int translucency = CINI::Art->GetInteger(artId, "Translucency");
        return static_cast<unsigned char>(std::clamp(255 - translucency * 256 / 100, 0, 255));
    }

    // Resolves the .SHP file name of a shape exactly like the game does when it
    // loads objects (see CLoadingExt::LoadObjects): NewTheater letter first, then
    // the generic letter, then every theater letter until the file exists.
    bool ResolveShapeFile(const FString& artId, const FString& imageName, FString& outFile)
    {
        auto pLoading = CLoadingExt::GetExtension();
        if (!pLoading)
            return false;

        bool applyNewTheater = CINI::Art->GetBool(artId, "NewTheater");
        applyNewTheater = CINI::Art->GetBool(imageName, "NewTheater", applyNewTheater);

        FString file = imageName + ".SHP";

        // Shapes taken from RA2(MD).mix always count as NewTheater.
        if (CLoadingExt::Ra2dotMixes.find(pLoading->HasFileMix(file)) != CLoadingExt::Ra2dotMixes.end())
            applyNewTheater = true;

        if (applyNewTheater)
            pLoading->SetTheaterLetter(file, ExtConfigs::NewTheaterType ? 1 : 0);

        if (!pLoading->HasFileExt(file))
        {
            pLoading->SetGenericTheaterLetter(file);
            if (!pLoading->HasFileExt(file))
            {
                if (ExtConfigs::UseStrictNewTheater)
                    return false;

                auto searchNewTheater = [pLoading, &file](char theater)
                {
                    if (file.GetLength() >= 2)
                        file.SetAt(1, theater);
                    return pLoading->HasFileExt(file);
                };

                file = imageName + ".SHP";
                auto& letters = TheaterHelpers::GetFileTheaterLetter();
                if (!searchNewTheater(letters['T']) &&
                    !searchNewTheater(letters['A']) &&
                    !searchNewTheater(letters['U']) &&
                    !searchNewTheater(letters['N']) &&
                    !searchNewTheater(letters['L']) &&
                    !searchNewTheater(letters['D']))
                {
                    return false;
                }
            }
        }

        outFile = file;
        return true;
    }

    // Expands art.ini fields into a frame sequence using the Animation Looping rules:
    //   Start       first frame of the first iteration (0 based)
    //   End         number of frames played from Start (not an end index)
    //   LoopStart   first frame of the loop segment (0 based)
    //   LoopEnd     last frame of the loop segment (1 based)
    //   LoopCount   total iterations (1 = once, >= 2 = N times, -1 = endless)
    std::vector<int> BuildSequence(const FString& artId, int usableCount, int& loopBegin, bool& infinite,
                                   std::vector<int>* iterationStarts = nullptr)
    {
        std::vector<int> seq;
        loopBegin = 0;
        infinite = false;
        if (iterationStarts)
            iterationStarts->clear();
        if (usableCount <= 0)
            return seq;

        int start = std::clamp(CINI::Art->GetInteger(artId, "Start", 0), 0, usableCount - 1);

        int end = CINI::Art->GetInteger(artId, "End", 0);
        if (end <= 0)
            end = usableCount - start;
        end = std::clamp(end, 1, usableCount - start);

        int loopCount = CINI::Art->GetInteger(artId, "LoopCount", 1);
        int loopStart = std::clamp(CINI::Art->GetInteger(artId, "LoopStart", 0), 0, usableCount - 1);
        int loopEnd = std::clamp(CINI::Art->GetInteger(artId, "LoopEnd", usableCount), 1, usableCount);

        const bool hasLoop = (loopCount >= 2 || loopCount < 0);

        // First iteration: Start -> LoopEnd-1 (Start+End-1 when there is no loop).
        int firstSegEnd = hasLoop ? (loopEnd - 1) : (start + end - 1);
        firstSegEnd = std::clamp(firstSegEnd, start, usableCount - 1);

        // Loop segment: LoopStart -> Start+End-1.
        int secondSegEnd = std::clamp(start + end - 1, loopStart, usableCount - 1);

        for (int i = start; i <= firstSegEnd; ++i)
            seq.push_back(i);

        // The first iteration always starts at the beginning of the sequence.
        if (iterationStarts && !seq.empty())
            iterationStarts->push_back(0);

        if (hasLoop)
        {
            loopBegin = static_cast<int>(seq.size());

            // LoopCount < 0: expand the loop segment once, then repeat it endlessly.
            const int repeatLen = secondSegEnd - loopStart + 1;
            int repeat = (loopCount < 0) ? 1 : (loopCount - 1);
            infinite = (loopCount < 0);
            for (int t = 0; t < repeat; ++t)
            {
                // Every repetition replays the animation, so it replays the report too.
                if (iterationStarts && repeatLen > 0)
                    iterationStarts->push_back(loopBegin + t * repeatLen);

                for (int i = loopStart; i <= secondSegEnd; ++i)
                    seq.push_back(i);
            }
        }

        // Fallback: when nothing matched, play the whole shape (shadow frames excluded).
        if (seq.empty())
        {
            for (int i = 0; i < usableCount; ++i)
                seq.push_back(i);
            loopBegin = 0;
            infinite = false;

            if (iterationStarts)
                iterationStarts->push_back(0);
        }

        if (loopBegin < 0 || loopBegin >= static_cast<int>(seq.size()))
            loopBegin = 0;

        return seq;
    }

    // Loads one shape of the chain and appends its frames and sequence entries.
    void AppendShape(const FString& artId, AnimFrames& out)
    {
        auto pLoading = CLoadingExt::GetExtension();
        if (!pLoading)
            return;

        const FString imageId = CINI::Art->GetString(artId, "Image", artId);

        FString fileName;
        if (!ResolveShapeFile(artId, imageId, fileName))
            return;
        if (!CMixFile::LoadSHP(fileName))
            return;

        ShapeHeader header;
        if (!CShpFile::GetSHPHeader(&header))
            return;
        if (header.Width <= 0 || header.Height <= 0 || header.FrameCount <= 0)
            return;

        FString palName = CINI::Art->GetString(artId, "CustomPalette", "anim.pal");
        pLoading->GetFullPaletteName(palName);
        Palette* pal = PalettesManager::LoadPalette(palName);
        if (!pal)
            pal = Palette::PALETTE_UNIT;

        const int frameOffset = static_cast<int>(out.Frames.size());
        for (int i = 0; i < header.FrameCount; ++i)
        {
            unsigned char* pBuffer = nullptr;
            CLoadingExt::LoadSHPFrameSafe(i, 1, &pBuffer, header);

            auto pData = std::make_unique<ImageDataClassSafe>();
            // SetImageDataSafe takes ownership of pBuffer (it must come from the
            // game allocator) and frees it internally, so it must not be freed here.
            pLoading->SetImageDataSafe(pBuffer, pData.get(), header.Width, header.Height, pal);
            out.Frames.push_back(std::move(pData));
        }

        // With Shadow=yes the second half of the SHP holds the shadow frames.
        int usableCount = header.FrameCount;
        if (CINI::Art->GetBool(artId, "Shadow", false) && header.FrameCount > 1)
            usableCount = header.FrameCount / 2;

        int loopBegin = 0;
        bool infinite = false;
        std::vector<int> iterationStarts;
        const std::vector<int> sequence = BuildSequence(artId, usableCount, loopBegin, infinite, &iterationStarts);

        const int sequenceStart = static_cast<int>(out.Sequence.size());
        const int delay = GetFrameDelayMs(artId);
        const unsigned char alpha = GetShapeAlpha(artId);
        for (int index : sequence)
        {
            out.Sequence.push_back(frameOffset + index);
            out.Delays.push_back(delay);
            out.Alphas.push_back(alpha);
        }

        // The shape's own Report= sound.
        FString report = CINI::Art->GetString(artId, "Report");

        // Every iteration of this shape starts with its first frame, and that is
        // where its report is played.
        for (int index : iterationStarts)
            out.ReportPoints.emplace_back(sequenceStart + index, report);

        // Only the last shape of the chain can loop endlessly (following Next
        // stops at LoopCount < 0), so its loop point is the one that matters.
        if (infinite)
        {
            out.LoopBegin = sequenceStart + loopBegin;
            out.Infinite = true;
        }
    }

    // Loads every shape of the Next chain, concatenating their frames so they are
    // played one after another in chain order. Shapes that fail to load are skipped.
    bool LoadFrames(const FString& animId, AnimFrames& out)
    {
        auto pLoading = CLoadingExt::GetExtension();
        if (!pLoading)
            return false;

        const AnimChain chain = ResolveAnimChain(animId);

        // SetImageDataSafe also updates CLoadingExt::TallestBuildingHeight and
        // CIsoViewExt::EXTRA_BORDER_BOTTOM, both of which take part in the map
        // layout. An animation is neither a building nor a map object, so keep
        // those untouched and restore them right after loading.
        const int savedTallestBuildingHeight = CLoadingExt::TallestBuildingHeight;
        const int savedExtraBorderBottom = CIsoViewExt::EXTRA_BORDER_BOTTOM;

        // Sequence index each chain entry starts at, used as the loop point when
        // the chain closes on itself.
        std::vector<int> sequenceStarts;
        sequenceStarts.reserve(chain.Ids.size());
        for (const auto& artId : chain.Ids)
        {
            sequenceStarts.push_back(static_cast<int>(out.Sequence.size()));
            AppendShape(artId, out);
        }

        CLoadingExt::TallestBuildingHeight = savedTallestBuildingHeight;
        CIsoViewExt::EXTRA_BORDER_BOTTOM = savedExtraBorderBottom;

        if (out.Sequence.empty())
            return false;

        // A self-closing chain repeats forever and restarts at the entry the cycle
        // returns to (A -> Next=B -> Next=A restarts at A). Chains ended by
        // LoopCount < 0 already carry the loop point of that shape.
        if (!chain.OwnLoopPoint
            && chain.LoopFrom >= 0 && chain.LoopFrom < static_cast<int>(sequenceStarts.size()))
        {
            out.LoopBegin = sequenceStarts[chain.LoopFrom];
            out.Infinite = true;
        }

        if (out.LoopBegin < 0 || out.LoopBegin >= static_cast<int>(out.Sequence.size()))
            out.LoopBegin = 0;

        return true;
    }

    AnimFrames* GetOrBuildFrames(const FString& animId)
    {
        if (g_Cached && g_CachedId == animId)
            return g_Cached;

        auto frames = std::make_unique<AnimFrames>();
        if (!LoadFrames(animId, *frames))
            return nullptr;

        // The DirectX texture cache is keyed by the image object address. A freshly
        // created object can land on the address of a released object and then
        // GetTexture() would return that object's texture. Drop anything bound to
        // these addresses so the first draw uses this animation's own textures.
        if (ExtConfigs::DirectXRendering && CIsoViewExt::g_pDX)
        {
            for (auto& up : frames->Frames)
            {
                if (up)
                    CIsoViewExt::g_pDX->RemoveTexturesFor(up.get());
            }
        }

        AnimFrames* raw = frames.get();
        g_AllFrames.push_back(std::move(frames));
        g_Cached = raw;
        g_CachedId = animId;
        return raw;
    }

    // ---------------------------------------------------------------------
    // Cache
    // ---------------------------------------------------------------------

    // Drops every cached animation. The frames are copies of SHP data that the
    // resource reload releases, and the DirectX texture cache is keyed by their
    // addresses, so nothing of it may survive a reload.
    void ClearFrameCache()
    {
        if (ExtConfigs::DirectXRendering && CIsoViewExt::g_pDX)
        {
            for (auto& up : g_AllFrames)
            {
                if (!up)
                    continue;
                for (auto& frame : up->Frames)
                {
                    if (frame)
                        CIsoViewExt::g_pDX->RemoveTexturesFor(frame.get());
                }
            }
        }

        g_AllFrames.clear();
        g_Cached = nullptr;
        g_CachedId = "";
    }

    // ---------------------------------------------------------------------
    // Scaled copies of the frames (only the DirectDraw path needs them)
    // ---------------------------------------------------------------------

    void BuildScaledFrames(AnimFrames& f, double inv)
    {
        auto pLoading = CLoadingExt::GetExtension();

        f.ScaledFrames.clear();
        f.ScaledInv = inv;
        f.ScaledFrames.reserve(f.Frames.size());

        for (auto& up : f.Frames)
        {
            auto* src = up.get();
            if (!src || !src->pImageBuffer || src->FullWidth <= 0 || src->FullHeight <= 0)
            {
                f.ScaledFrames.push_back(nullptr);
                continue;
            }

            const int sw = src->FullWidth;
            const int sh = src->FullHeight;
            const int dw = std::max(1, static_cast<int>(std::lround(sw * inv)));
            const int dh = std::max(1, static_cast<int>(std::lround(sh * inv)));

            // Must use the game allocator: SetImageDataSafe takes ownership and frees it.
            unsigned char* pBuffer = GameCreateArray<unsigned char>(static_cast<size_t>(dw) * dh);
            if (!pBuffer)
            {
                f.ScaledFrames.push_back(nullptr);
                continue;
            }

            for (int y = 0; y < dh; ++y)
            {
                const int sy = std::min(sh - 1, static_cast<int>(y / inv));
                const unsigned char* srcRow = src->pImageBuffer.get() + static_cast<size_t>(sy) * sw;
                unsigned char* dstRow = pBuffer + static_cast<size_t>(y) * dw;
                for (int x = 0; x < dw; ++x)
                    dstRow[x] = srcRow[std::min(sw - 1, static_cast<int>(x / inv))];
            }

            auto dst = std::make_unique<ImageDataClassSafe>();
            if (pLoading)
            {
                pLoading->SetImageDataSafe(pBuffer, dst.get(), dw, dh, src->pPalette);
            }
            else
            {
                GameDeleteArray(pBuffer, static_cast<size_t>(dw) * dh);
                f.ScaledFrames.push_back(nullptr);
                continue;
            }
            f.ScaledFrames.push_back(std::move(dst));
        }
    }

    const std::vector<std::unique_ptr<ImageDataClassSafe>>& GetDrawFrames(AnimFrames& f, double inv)
    {
        if (inv == 1.0)
            return f.Frames;

        if (f.ScaledInv != inv || f.ScaledFrames.size() != f.Frames.size())
            BuildScaledFrames(f, inv);

        return f.ScaledFrames;
    }

    // Per-frame attributes: every shape of the chain has its own Rate and
    // Translucency, so both are looked up for the frame shown right now.
    int CurrentFrameDelayMs()
    {
        if (!g_Current || g_Current->Delays.empty())
            return MIN_INTERVAL_MS;

        const int index = std::clamp(g_Pos, 0, static_cast<int>(g_Current->Delays.size()) - 1);
        return g_Current->Delays[index];
    }

    unsigned char CurrentFrameAlpha()
    {
        if (!g_Current || g_Current->Alphas.empty())
            return 255;

        const int index = std::clamp(g_Pos, 0, static_cast<int>(g_Current->Alphas.size()) - 1);
        return g_Current->Alphas[index];
    }

    // True when the playback has at least one report sound to play.
    bool HasAnyReport(const AnimFrames& frames)
    {
        for (const auto& point : frames.ReportPoints)
        {
            if (!point.second.IsEmpty())
                return true;
        }
        return false;
    }

    // Report sound of the shape that owns the given sequence position. Positions
    // that are not the first frame of a shape (or of one of its iterations) have
    // no report.
    const FString* GetReportAt(const AnimFrames& frames, int pos)
    {
        for (const auto& point : frames.ReportPoints)
        {
            if (point.first == pos && !point.second.IsEmpty())
                return &point.second;
        }
        return nullptr;
    }

    // ---------------------------------------------------------------------
    // DirectDraw: clean background and present
    // ---------------------------------------------------------------------

    // Restores the visible area from the game's TempBuffer, exactly like the
    // mouse-move flow does (Hooks.Zoom.cpp, CIsoView_OnMouseMove_BltTempBuffer).
    void RestoreCleanRegion()
    {
        auto pIsoView = CIsoView::GetInstance();
        if (!pIsoView || !pIsoView->lpDDTempBufferSurface)
            return;

        auto pBackBuffer = CIsoViewExt::GetBackBuffer();
        if (!pBackBuffer)
            return;

        CRect rect = CIsoViewExt::GetVisibleIsoViewRect();
        pBackBuffer->Blt(&rect, pIsoView->lpDDTempBufferSurface, &rect, DDBLT_WAIT, 0);
    }

    // Matches the output of the BACK_BUFFER_TO_PRIMARY macro for special_draw >= 1.
    void PresentBackBuffer()
    {
        auto pIsoView = CIsoView::GetInstance();
        if (!pIsoView || !pIsoView->lpDDPrimarySurface)
            return;

        CRect dr = CIsoViewExt::GetVisibleIsoViewRect();

        if (ExtConfigs::SecondScreenSupport)
        {
            CRect drFixed = dr;
            CIsoViewExt::BltToWindow(pIsoView->m_hWnd, CIsoViewExt::GetBackBuffer(), &dr, &drFixed);
        }
        else
        {
            pIsoView->lpDDPrimarySurface->Blt(&dr, CIsoViewExt::GetBackBuffer(), &dr, DDBLT_WAIT, 0);
        }
    }

    // ---------------------------------------------------------------------
    // Drawing
    // ---------------------------------------------------------------------

    // DirectDraw: blits a (pre-scaled) frame into the BackBuffer.
    void DrawFrameGDI(ImageDataClassSafe* pFrame, int x, int y, unsigned char alpha)
    {
        auto pBackBuffer = CIsoViewExt::GetBackBuffer();
        if (!pBackBuffer)
            return;

        DDSURFACEDESC2 ddsd = { sizeof(DDSURFACEDESC2) };
        if (FAILED(pBackBuffer->Lock(nullptr, &ddsd, DDLOCK_WAIT, nullptr)))
            return;

        if (ddsd.lpSurface)
        {
            RECT window{ 0, 0, (LONG)ddsd.dwWidth, (LONG)ddsd.dwHeight };
            DDBoundary boundary{ ddsd.dwWidth, ddsd.dwHeight, ddsd.lPitch };
            // extraLightType = -100 skips lighting, same as the damage fires in DrawObjects.
            CIsoViewExt::BlitSHPTransparent(CIsoView::GetInstance(), ddsd.lpSurface, window, boundary,
                x, y, pFrame, nullptr, alpha, 0, -100, false);
        }

        pBackBuffer->Unlock(nullptr);
    }

    // Screen space is 1 unit per client pixel, while map objects are drawn in a
    // space of 1/ScaledFactor client pixels, so sizes and offsets are divided by
    // ScaledFactor to keep the same apparent scale as the map.
    void DrawFrameDirectX(ImageDataClassSafe* pFrame, int screenX, int screenY, unsigned char alpha)
    {
        if (!CIsoViewExt::DirectXReady() || !CIsoViewExt::g_pDX)
            return;

        auto pTexture = pFrame->GetTexture();
        if (!pTexture)
            return;

        const double sf = (CIsoViewExt::ScaledFactor > 0.0) ? CIsoViewExt::ScaledFactor : 1.0;
        const float inv = static_cast<float>(1.0 / sf);

        const float w = pFrame->FullWidth * inv;
        const float h = pFrame->FullHeight * inv;
        const float centerX = screenX + BLIT_X_OFFSET * inv;
        const float centerY = screenY + (BLIT_Y_OFFSET + ANCHOR_Y_OFFSET) * inv;

        // DirectXCore sizes the quad as texWidth * scale (texWidth comes from the
        // texture's own sourceView). The texture cache keys on the image address
        // and does not verify sizes, so compensate here to always draw the frame
        // at its intended screen size.
        const float texW = static_cast<float>(pTexture->sourceView.FullWidth);
        const float texH = static_cast<float>(pTexture->sourceView.FullHeight);
        if (texW <= 0.0f || texH <= 0.0f)
            return;
        const float scaleX = inv * (static_cast<float>(pFrame->FullWidth) / texW);
        const float scaleY = inv * (static_cast<float>(pFrame->FullHeight) / texH);

        // Drop leftover commands: RenderScreenSpaceOnly returns early when the
        // background cache is invalid, and such a command would otherwise be
        // rendered later by a pass with a different viewport/projection.
        CIsoViewExt::g_pDX->DiscardPendingDraws();

        DrawParams params;
        params.SetPosition(centerX - w * 0.5f, centerY - h * 0.5f)
            .SetScale(scaleX, scaleY)
            .SetOpacity(alpha / 255.0f)
            .SetScreenSpace();

        CIsoViewExt::g_pDX->DrawTexture(pTexture, params);
        // RenderScreenSpaceOnly restores the background cache before drawing the
        // screen-space content, so drawing one frame per call leaves no residue.
        CIsoViewExt::g_pDX->RenderScreenSpaceOnly();
    }

    void Redraw()
    {
        if (!g_Playing || !g_Current || g_Current->Sequence.empty())
            return;

        auto pIsoView = CIsoView::GetInstance();
        if (!pIsoView || !pIsoView->GetSafeHwnd())
            return;

        // Stay out of full-map and screenshot rendering.
        if (CIsoViewExt::RenderingMap || CIsoViewExt::RenderFullMap || CIsoViewExt::RenderingScreenshot)
            return;
        if (!::IsWindowVisible(pIsoView->GetSafeHwnd()))
            return;
        if (CFinalSunDlg::Instance && ::IsIconic(CFinalSunDlg::Instance->GetSafeHwnd()))
            return;

        const int frameIndex = g_Current->Sequence[g_Pos];
        if (frameIndex < 0 || frameIndex >= static_cast<int>(g_Current->Frames.size()))
            return;

        const double sf = (CIsoViewExt::ScaledFactor > 0.0) ? CIsoViewExt::ScaledFactor : 1.0;
        const double inv = 1.0 / sf;

        // Screen space position: Ext conversion plus the draw offsets (the DirectX
        // offsets include the window's left border).
        int sx = g_Anchor.X;
        int sy = g_Anchor.Y;
        CIsoViewExt::MapCoord2ScreenCoord(sx, sy);
        sx -= CIsoViewExt::drawOffsetX;
        sy -= CIsoViewExt::drawOffsetY;

        // art.ini Translucency of the shape this frame belongs to.
        const unsigned char alpha = CurrentFrameAlpha();

        if (ExtConfigs::DirectXRendering)
        {
            auto* pFrame = g_Current->Frames[frameIndex].get();
            if (!ImageDataClassSafe::IsValidImage(pFrame))
                return;

            DrawFrameDirectX(pFrame, sx, sy, alpha);
            return;
        }

        // DirectDraw: draw the pre-scaled frame 1:1 into the BackBuffer.
        auto& drawFrames = GetDrawFrames(*g_Current, inv);
        if (frameIndex >= static_cast<int>(drawFrames.size()))
            return;

        auto* pFrame = drawFrames[frameIndex].get();
        if (!ImageDataClassSafe::IsValidImage(pFrame))
            return;

        const float invF = static_cast<float>(inv);
        const int offX = static_cast<int>(std::lround(BLIT_X_OFFSET * invF));
        const int offY = static_cast<int>(std::lround(BLIT_Y_OFFSET * invF));
        const int anchorY = static_cast<int>(std::lround(ANCHOR_Y_OFFSET * invF));

        // BlitSHPTransparent expects the top-left corner and adds the BLIT offsets
        // internally, so remove them here and re-add the scaled screen-space ones.
        const int blitX = sx + offX - pFrame->FullWidth / 2 - BLIT_X_OFFSET;
        const int blitY = sy + offY + anchorY - pFrame->FullHeight / 2 - BLIT_Y_OFFSET;

        // Erase the previous frame by putting the clean canvas back, then draw the
        // new one on top of it.
        RestoreCleanRegion();
        DrawFrameGDI(pFrame, blitX, blitY, alpha);
        PresentBackBuffer();
    }

    // ---------------------------------------------------------------------
    // Report sound (same steps as the Sound parameter jump in CNewScript)
    // ---------------------------------------------------------------------

    void PlayReportSound(const FString& soundListId)
    {
        if (soundListId.IsEmpty())
            return;

        auto soundNames = CINI::Sound->GetString(soundListId, "Sounds");
        soundNames.Trim();
        auto sounds = FString::SplitString(soundNames, " ");
        if (sounds.empty())
            return;

        auto randomSound = STDHelpers::RandomSelect(sounds);
        randomSound.Trim();
        if (randomSound.IsEmpty())
            return;

        if (randomSound[0] == '$')
            randomSound = randomSound.Mid(1);

        const int volume = CINI::Sound->GetInteger(soundListId, "Volume", 100);

        g_ReportStarted = true;
        g_ReportName = randomSound;
        SoundPlayer::SetJumpTarget(REPORT_SOUND_SOURCE, 0, g_ReportName);
        SoundPlayer::PlayBagSound(g_ReportName, volume);
    }

    // Stops the report sound started by this playback. SoundPlayer is shared, so
    // another jump target means the script owns the sound now and it is left alone.
    void StopReportSound()
    {
        if (!g_ReportStarted)
            return;

        g_ReportStarted = false;

        if (SoundPlayer::IsPlaying()
            && SoundPlayer::IsSameJumpTarget(REPORT_SOUND_SOURCE, 0, g_ReportName))
        {
            SoundPlayer::Stop();
        }

        g_ReportName = "";
    }

    // Ends the current playback. The report sound is only stopped when asked to,
    // so starting an animation without a report leaves the running one alone.
    void StopInternal(bool stopReportSound)
    {
        if (CFinalSunDlg::Instance)
            ::KillTimer(CFinalSunDlg::Instance->GetSafeHwnd(), TIMER_ID);

        if (stopReportSound)
            StopReportSound();

        g_Playing = false;
        g_Current = nullptr;
        g_PlayingId = "";
        g_Pos = 0;
    }

    // Erases the current frame and ends playback.
    void StopPlayback(bool stopReportSound)
    {
        if (!g_Playing)
        {
            // Playback already ended, but an earlier report may still be running.
            if (stopReportSound)
                StopReportSound();
            return;
        }

        // Restore the canvas, otherwise the last frame would stay on screen.
        if (!CIsoViewExt::RenderingMap && !CIsoViewExt::RenderFullMap && !CIsoViewExt::RenderingScreenshot)
        {
            if (ExtConfigs::DirectXRendering)
            {
                // DirectX: the overlay lives in screen space, so re-publishing the
                // background cache removes it.
                if (CIsoViewExt::DirectXReady() && CIsoViewExt::g_pDX)
                    CIsoViewExt::g_pDX->RenderScreenSpaceOnly();
            }
            else
            {
                RestoreCleanRegion();
                PresentBackBuffer();
            }
        }

        StopInternal(stopReportSound);
    }

    // The timer is re-armed after every frame because shapes in the chain may
    // have different Rates.
    void RearmTimer()
    {
        if (CFinalSunDlg::Instance)
            ::SetTimer(CFinalSunDlg::Instance->GetSafeHwnd(), TIMER_ID, CurrentFrameDelayMs(), nullptr);
    }
}

// -------------------------------------------------------------------------
// Public interface
// -------------------------------------------------------------------------

bool Play(const FString& animId, MapCoord coord)
{
    if (animId.IsEmpty())
        return false;

    if (!CMapData::Instance->MapWidthPlusHeight)
        return false;

    AnimFrames* frames = GetOrBuildFrames(animId);
    if (!frames)
        return false;

    // Every playback starts its own reports. When the new animation has none, the
    // report still running from the previous one is left untouched.
    StopPlayback(!HasAnyReport(*frames));

    g_Current = frames;
    g_PlayingId = animId;
    g_Anchor = coord;
    g_Pos = 0;
    g_Playing = true;

    // The first frame plays the report of the shape it belongs to.
    if (const FString* report = GetReportAt(*frames, 0))
        PlayReportSound(*report);

    Redraw();

    // The first frame is on screen; the timer advances from here on and stops
    // playback when the sequence ends.
    RearmTimer();

    return true;
}

void Stop()
{
    StopPlayback(true);
}

void ClearCache()
{
    // No surface work here: the caller is reloading resources, so the DirectDraw
    // surfaces may be gone or about to be recreated. The report sound does belong to
    // the released resources, so it is stopped as well.
    StopInternal(true);
    ClearFrameCache();
}

bool IsPlaying()
{
    return g_Playing;
}

bool IsSame(const FString& animId)
{
    return g_Playing && g_PlayingId == animId;
}

bool IsTimerMessage(UINT_PTR timerId)
{
    return timerId == TIMER_ID;
}

void OnTimer()
{
    if (!g_Playing || !g_Current || g_Current->Sequence.empty())
        return;

    ++g_Pos;
    if (g_Pos >= static_cast<int>(g_Current->Sequence.size()))
    {
        // LoopCount decides the end: only LoopCount<0 keeps looping.
        if (g_Current->Infinite)
        {
            g_Pos = g_Current->LoopBegin;
        }
        else
        {
            Stop();
            return;
        }
    }

    // Reaching the first frame of a shape (or of one of its iterations, e.g. when a
    // loop wraps around) plays that shape's report, so A->B plays A's, then B's.
    if (const FString* report = GetReportAt(*g_Current, g_Pos))
        PlayReportSound(*report);

    Redraw();
    // The new frame may belong to a shape with a different Rate.
    RearmTimer();
}

void OnUserInterrupt()
{
    if (!g_Playing)
        return;

    // Called from SpecialDraw / SpecialDrawDirectX: the caller's rendering flow
    // repaints the visible area right after this, so nothing has to be erased.
    StopInternal(true);
}
}
