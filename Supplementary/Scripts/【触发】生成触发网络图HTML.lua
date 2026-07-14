-- 生成触发关系网络图（HTML + D3.js）
-- 输出文件：trigger_network.html（位于FA2spHE运行目录）

-- 辅助函数：HTML转义（对<,>,&等）
local function html_escape(s)
    if not s then return "" end
    s = string.gsub(s, "&", "&amp;")
    s = string.gsub(s, "<", "&lt;")
    s = string.gsub(s, ">", "&gt;")
    s = string.gsub(s, '"', "&quot;")
    s = string.gsub(s, "'", "&#39;")
    return s
end

-- 辅助函数：JSON字符串转义（用于JavaScript字符串）
local function json_escape(s)
    if not s then return "" end
    s = string.gsub(s, "\\", "\\\\")
    s = string.gsub(s, '"', '\\"')
    s = string.gsub(s, "\n", "\\n")
    s = string.gsub(s, "\r", "\\r")
    s = string.gsub(s, "\t", "\\t")
    return s
end

-- 获取所有触发ID
local trig_ids = get_triggers()
if #trig_ids == 0 then
    print("地图中没有触发器。")
    return
end

-- 建立映射：ID -> 名称、Disabled状态
local trig_names = {}
local disabled = {}
for _, id in ipairs(trig_ids) do
    local name = get_param("Triggers", id, 3, ",") or ""
    if name == "" then
        name = id
    end
    trig_names[id] = name
    local dis_str = get_param("Triggers", id, 4, ",") or "0"
    disabled[id] = (dis_str == "1")
end

-- 构建允许边（Action 53）
local allow_edges = {}
for _, id in ipairs(trig_ids) do
    allow_edges[id] = {}
end

for _, id in ipairs(trig_ids) do
    local trig = get_trigger(id)
    if trig then
        for _, action_str in ipairs(trig.actions) do
            local parts = split_string(action_str, ",")
            if #parts >= 3 then
                local action_id = tonumber(parts[1])
                if action_id == 53 then
                    local target = parts[3]
                    if target and target ~= "" and trig_names[target] then
                        table.insert(allow_edges[id], target)
                    end
                end
            end
        end
    end
end

-- 构建节点数据
local nodes = {}
local node_index = {}
for i, id in ipairs(trig_ids) do
    node_index[id] = i - 1
    local display_name = html_escape(trig_names[id])  -- 仅转义HTML敏感字符
    table.insert(nodes, {
        id = i - 1,
        label = display_name,
        disabled = disabled[id] and 1 or 0,
        raw_name = trig_names[id]  -- 原始名称，用于工具提示
    })
end

-- 构建边数据
local edges = {}
for _, from in ipairs(trig_ids) do
    for _, to in ipairs(allow_edges[from]) do
        table.insert(edges, {
            source = node_index[from],
            target = node_index[to],
            type = "allow"
        })
    end
end

-- 手动构建JSON（保证正确转义）
local function build_json_nodes()
    local parts = {}
    for _, n in ipairs(nodes) do
        local label = json_escape(n.label)      -- 转义JSON字符串
        local raw = json_escape(n.raw_name)
        table.insert(parts, string.format('{"id":%d,"label":"%s","disabled":%d,"raw_name":"%s"}', 
            n.id, label, n.disabled, raw))
    end
    return "[" .. table.concat(parts, ",") .. "]"
end

local function build_json_edges()
    local parts = {}
    for _, e in ipairs(edges) do
        table.insert(parts, string.format('{"source":%d,"target":%d,"type":"%s"}', 
            e.source, e.target, e.type))
    end
    return "[" .. table.concat(parts, ",") .. "]"
end

local nodes_json = build_json_nodes()
local edges_json = build_json_edges()

-- 节点数量自适应力参数
local node_count = #nodes
local link_distance = 150
local charge_strength = -300
local collide_radius = 40
if node_count > 50 then
    link_distance = 120
    charge_strength = -200
    collide_radius = 30
end
if node_count > 100 then
    link_distance = 100
    charge_strength = -150
    collide_radius = 25
end
if node_count > 200 then
    link_distance = 80
    charge_strength = -100
    collide_radius = 20
end

-- 生成HTML模板
local html_template = [[
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>触发器允许关系网络图</title>
    <style>
        body { margin: 0; overflow: hidden; background: #f8f8f8; font-family: "Microsoft YaHei", sans-serif; }
        .node rect {
            stroke: #333;
            stroke-width: 1.5px;
            fill: #fff;
            rx: 5;
        }
        .node text {
            font-size: 12px;
            fill: #333;
            pointer-events: none;
            text-anchor: middle;
            dominant-baseline: central;
        }
        .edge path {
            stroke: #555;
            stroke-width: 2px;
            fill: none;
            marker-end: url(#arrowhead);
        }
        .node.enabled rect { fill: #d9edf7; }
        .node.disabled rect { fill: #f2dede; }
        .tooltip {
            position: absolute;
            background: rgba(0,0,0,0.8);
            color: #fff;
            padding: 5px 10px;
            border-radius: 4px;
            pointer-events: none;
            font-size: 14px;
            max-width: 300px;
            word-wrap: break-word;
        }
        #reset-btn {
            position: absolute;
            top: 20px;
            right: 20px;
            padding: 8px 16px;
            background: #337ab7;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
            z-index: 100;
            box-shadow: 0 2px 4px rgba(0,0,0,0.2);
        }
        #reset-btn:hover { background: #286090; }
        #info {
            position: absolute;
            bottom: 20px;
            left: 20px;
            color: #888;
            font-size: 12px;
            z-index: 100;
            background: rgba(255,255,255,0.8);
            padding: 4px 10px;
            border-radius: 4px;
        }
    </style>
</head>
<body>
<button id="reset-btn">重置视图</button>
<div id="info">滚轮缩放 | 拖拽平移 | 拖动节点</div>
<div id="graph"></div>
<script src="https://d3js.org/d3.v7.min.js"></script>
<script>
    var nodes = NODE_DATA;
    var edges = EDGE_DATA;

    var width = window.innerWidth;
    var height = window.innerHeight;

    var zoom = d3.zoom()
        .scaleExtent([0.1, 3])
        .on("zoom", function(event) {
            svgGroup.attr("transform", event.transform);
        });

    var svg = d3.select("#graph")
        .append("svg")
        .attr("width", width)
        .attr("height", height)
        .call(zoom);

    var svgGroup = svg.append("g");

    var defs = svg.append("defs");
    defs.append("marker")
        .attr("id", "arrowhead")
        .attr("viewBox", "0 0 10 10")
        .attr("refX", 10)
        .attr("refY", 5)
        .attr("markerWidth", 8)
        .attr("markerHeight", 8)
        .attr("orient", "auto")
        .append("path")
        .attr("d", "M 0 0 L 10 5 L 0 10 z")
        .attr("fill", "#555");

    var simulation = d3.forceSimulation(nodes)
        .force("link", d3.forceLink(edges).id(function(d) { return d.id; }).distance(LINK_DIST))
        .force("charge", d3.forceManyBody().strength(CHARGE_STR))
        .force("center", d3.forceCenter(width/2, height/2))
        .force("collide", d3.forceCollide(COLLIDE_RADIUS));

    var link = svgGroup.append("g")
        .selectAll(".edge")
        .data(edges)
        .enter().append("g")
        .attr("class", "edge")
        .append("path");

    var node = svgGroup.append("g")
        .selectAll(".node")
        .data(nodes)
        .enter().append("g")
        .attr("class", function(d) { return "node" + (d.disabled ? " disabled" : " enabled"); })
        .call(d3.drag()
            .on("start", dragstarted)
            .on("drag", dragged)
            .on("end", dragended)
        );

    node.append("rect")
        .attr("width", function(d) { return Math.max(d.label.length * 8 + 20, 40); })
        .attr("height", 30)
        .attr("x", function(d) { return -(Math.max(d.label.length * 8 + 20, 40))/2; })
        .attr("y", -15);

    node.append("text")
        .text(function(d) { return d.label; });

    var tooltip = d3.select("body").append("div")
        .attr("class", "tooltip")
        .style("opacity", 0);

    node.on("mouseover", function(event, d) {
        var info = "ID: " + d.id + "<br/>名称: " + d.raw_name + "<br/>状态: " + (d.disabled ? "禁用" : "启用");
        tooltip.html(info)
            .style("left", (event.pageX + 10) + "px")
            .style("top", (event.pageY - 20) + "px")
            .transition().duration(200).style("opacity", 0.9);
    })
    .on("mouseout", function() {
        tooltip.transition().duration(500).style("opacity", 0);
    });

    simulation.on("tick", function() {
        link.attr("d", function(d) {
            var dx = d.target.x - d.source.x;
            var dy = d.target.y - d.source.y;
            var dr = Math.sqrt(dx*dx + dy*dy);
            var offset = 20;
            var ratio = (dr - offset) / dr;
            var sx = d.source.x + dx * (1 - ratio);
            var sy = d.source.y + dy * (1 - ratio);
            var tx = d.target.x - dx * (1 - ratio);
            var ty = d.target.y - dy * (1 - ratio);
            return "M" + sx + "," + sy + "L" + tx + "," + ty;
        });
        node.attr("transform", function(d) {
            return "translate(" + d.x + "," + d.y + ")";
        });
    });

    function dragstarted(event, d) {
        if (!event.active) simulation.alphaTarget(0.3).restart();
        d.fx = d.x;
        d.fy = d.y;
    }
    function dragged(event, d) {
        d.fx = event.x;
        d.fy = event.y;
    }
    function dragended(event, d) {
        if (!event.active) simulation.alphaTarget(0);
    }

    document.getElementById("reset-btn").addEventListener("click", function() {
        var bbox = { x1: Infinity, y1: Infinity, x2: -Infinity, y2: -Infinity };
        nodes.forEach(function(d) {
            if (d.x < bbox.x1) bbox.x1 = d.x;
            if (d.y < bbox.y1) bbox.y1 = d.y;
            if (d.x > bbox.x2) bbox.x2 = d.x;
            if (d.y > bbox.y2) bbox.y2 = d.y;
        });
        var margin = 50;
        var w = bbox.x2 - bbox.x1 + 2*margin;
        var h = bbox.y2 - bbox.y1 + 2*margin;
        var scale = Math.min(width / w, height / h, 1.5);
        var cx = (bbox.x1 + bbox.x2) / 2;
        var cy = (bbox.y1 + bbox.y2) / 2;
        var transform = d3.zoomIdentity
            .translate(width/2 - cx*scale, height/2 - cy*scale)
            .scale(scale);
        svg.transition().duration(750).call(zoom.transform, transform);
    });

    window.addEventListener("resize", function() {
        width = window.innerWidth;
        height = window.innerHeight;
        svg.attr("width", width).attr("height", height);
        simulation.force("center", d3.forceCenter(width/2, height/2));
        simulation.restart();
    });

    setTimeout(function() {
        document.getElementById("reset-btn").click();
    }, 1000);
</script>
</body>
</html>
]]

-- 替换动态参数
local html_content = html_template
html_content = string.gsub(html_content, "NODE_DATA", nodes_json)
html_content = string.gsub(html_content, "EDGE_DATA", edges_json)
html_content = string.gsub(html_content, "LINK_DIST", tostring(link_distance))
html_content = string.gsub(html_content, "CHARGE_STR", tostring(charge_strength))
html_content = string.gsub(html_content, "COLLIDE_RADIUS", tostring(collide_radius))

-- 写入文件
local file_path = exe_path() .. "\\trigger_network.html"
local file = io.open(file_path, "w")
if file then
	html_content = to_utf8(html_content)
    file:write(html_content)
    file:close()
    print("HTML文件已生成: " .. file_path)
    -- 自动在默认浏览器中打开（调用cmd的start命令）
    local ret = exec("trigger_network.html", { file = true })
    if ret then
        print("已在浏览器中打开: " .. file_path)
    else
        print("自动打开失败，请手动用浏览器打开: " .. file_path)
        message_box("触发网络图已生成！\n文件路径: " .. file_path .. "\n请用浏览器打开查看。\n支持滚轮缩放、拖拽平移、拖动节点。", "成功", 1)
    end
else
    print("写入文件失败！")
end