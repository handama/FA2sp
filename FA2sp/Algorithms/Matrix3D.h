#pragma once
#include <cmath>

class Matrix3D {
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double SIN_60 = 0.8660254037844386; 
    static constexpr double INV_SQRT2 = 0.7071067811865476; 
    static constexpr double SCALE = 30.0 * 1.4142135623730951 / 256.0;

public:
    Matrix3D(double f, double l, double h, int facing, int facingCount,
        double tiltAngle = 0.0, double tiltDirection = 0.0, double fireAngle = 0.0)
    {
        OutputX = 0.0;
        OutputY = 0.0;
        if (f != 0.0 || l != 0.0 || h != 0.0)
            Transform(f, l, h, facing, facingCount, tiltAngle, tiltDirection, fireAngle);
    }

private:
    void Transform(double F, double L, double H, int facing, int facingCount,
        double tiltAngle, double tiltDirection, double fireAngle)
    {
        double px = F * SCALE;
        double py = L * SCALE;
        double pz = H * SCALE;

        px = -px;

        if (fireAngle != 0.0)
        {
            double angleFire = fireAngle / 64.0 * 90.0 / 360.0 * 2.0 * PI;
            double c = cos(angleFire);
            double s = sin(angleFire);
            double nx = px * c + pz * s;
            double nz = -px * s + pz * c;
            px = nx;
            pz = nz;
        }

        int actFacing = (facing + facingCount - 2 * facingCount / 8) % facingCount;
        int diridx = (facingCount == 32) ? actFacing : actFacing * 4;
        double angle = (diridx + 16) % 32 * (2.0 * PI / 32.0);
        if (angle != 0.0)
        {
            double c = cos(angle);
            double s = sin(angle);
            double nx = px * c - py * s;
            double ny = px * s + py * c;
            px = nx;
            py = ny;
        }

        if (tiltAngle != 0.0)
        {
            double cos_d = cos(tiltDirection);
            double sin_d = sin(tiltDirection);
            double c = cos(tiltAngle);
            double s = -sin(tiltAngle);
            double one_minus_c = 1.0 - c;

            double m00 = 1.0 - one_minus_c * cos_d * cos_d;
            double m01 = -one_minus_c * cos_d * sin_d;
            double m02 = s * cos_d;
            double m10 = -one_minus_c * sin_d * cos_d;
            double m11 = 1.0 - one_minus_c * sin_d * sin_d;
            double m12 = s * sin_d;
            double m20 = -s * cos_d;
            double m21 = -s * sin_d;
            double m22 = c;

            double nx = px * m00 + py * m10 + pz * m20;
            double ny = px * m01 + py * m11 + pz * m21;
            double nz = px * m02 + py * m12 + pz * m22;
            px = nx;
            py = ny;
            pz = nz;
        }

        OutputX = (px - py) * INV_SQRT2;
        OutputY = (px + py) * 0.5 * INV_SQRT2 - pz * SIN_60;
    }

public:
    double OutputX;
    double OutputY;
};
