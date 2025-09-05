--【路径点】在两个路径点之间生成间隔均匀的路径点.lua
--喵---喵---喵---喵--



input_N = input_box("输入生成的路径点间隔")
N = tonumber(input_N)
if N < 1 or N >= 100 then
message_box("输入数据不合要求, 默认将生成的路径点间隔调整为15", "输入内容非法", 1)
N = 15
end

input_wp1 = input_box("输入生成的起点路径点")
wp1_value = tonumber(get_string("Waypoints", input_wp1))
wp1_y = math.floor((wp1_value)/1000)
wp1_x = wp1_value - 1000 * wp1_y
print("起点为：#"..input_wp1)
print("起点为：("..wp1_x..","..wp1_y..")")

input_wp2 = input_box("输入生成的路径点终点")
wp2_value = tonumber(get_string("Waypoints", input_wp2))
wp2_y = math.floor((wp2_value)/1000)
wp2_x = wp2_value - 1000 * wp2_y
print("终点为：#"..input_wp2)
print("终点为：("..wp2_x..","..wp2_y..")")

 points = {}
    -- 添加起点 A
    table.insert(points, {x = wp1_x, y = wp1_y})  
    -- 计算坐标差值
 dx = wp2_x - wp1_x
 dy = wp2_y - wp1_y
    -- 计算切比雪夫距离
 abs = math.abs
 adx = abs(dx)
 ady = abs(dy)
D = math.max(adx, ady)
    -- 处理重合点
    if D == 0 then
        return points
    end
    -- 计算步数（不包括起点）
 step_count = math.floor(D / N)
    if step_count == 0 then
        return points
    end
    -- 生成中间点
    for k = 1, step_count do
        -- 计算当前步长比例
 ratio = (k * N) / D
        -- 计算坐标并四舍五入取整
 x = wp1_x + math.floor(dx * ratio + 0.5)
 y = wp1_y + math.floor(dy * ratio + 0.5)
        
	new_wp = place_waypoint(x, y)
	print("放置的路径点为：#"..new_wp.."("..x..","..y..")")
end
update_waypoint()
message_box("已成功执行脚本", "执行成功", 1)

