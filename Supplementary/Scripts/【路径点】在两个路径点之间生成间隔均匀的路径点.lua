--��·���㡿������·����֮�����ɼ�����ȵ�·����.lua
--��---��---��---��--



input_N = input_box("�������ɵ�·������")
N = tonumber(input_N)
if N < 1 or N >= 100 then
message_box("�������ݲ���Ҫ��, Ĭ�Ͻ����ɵ�·����������Ϊ15", "�������ݷǷ�", 1)
N = 15
end

input_wp1 = input_box("�������ɵ����·����")
wp1_value = tonumber(get_string("Waypoints", input_wp1))
wp1_y = math.floor((wp1_value)/1000)
wp1_x = wp1_value - 1000 * wp1_y
print("���Ϊ��#"..input_wp1)
print("���Ϊ��("..wp1_x..","..wp1_y..")")

input_wp2 = input_box("�������ɵ�·�����յ�")
wp2_value = tonumber(get_string("Waypoints", input_wp2))
wp2_y = math.floor((wp2_value)/1000)
wp2_x = wp2_value - 1000 * wp2_y
print("�յ�Ϊ��#"..input_wp2)
print("�յ�Ϊ��("..wp2_x..","..wp2_y..")")

 points = {}
    -- ������ A
    table.insert(points, {x = wp1_x, y = wp1_y})  
    -- ���������ֵ
 dx = wp2_x - wp1_x
 dy = wp2_y - wp1_y
    -- �����б�ѩ�����
 abs = math.abs
 adx = abs(dx)
 ady = abs(dy)
D = math.max(adx, ady)
    -- �����غϵ�
    if D == 0 then
        return points
    end
    -- ���㲽������������㣩
 step_count = math.floor(D / N)
    if step_count == 0 then
        return points
    end
    -- �����м��
    for k = 1, step_count do
        -- ���㵱ǰ��������
 ratio = (k * N) / D
        -- �������겢��������ȡ��
 x = wp1_x + math.floor(dx * ratio + 0.5)
 y = wp1_y + math.floor(dy * ratio + 0.5)
        
	new_wp = place_waypoint(x, y)
	print("���õ�·����Ϊ��#"..new_wp.."("..x..","..y..")")
end
update_waypoint()
message_box("�ѳɹ�ִ�нű�", "ִ�гɹ�", 1)

