input = input_box('������·���㣬֧������������������룬��\n20-24,19,30')
guard = tonumber(input_box('�����뾯��ʱ�䣨��λ�룩'))
result = {}
for _, part in pairs(split_string(input, ",")) do
    local range_parts = split_string(part, "-")
    if #range_parts == 2 then
        local start_num, end_num = tonumber(range_parts[1]), tonumber(range_parts[2])
        if start_num and end_num then
            for i = start_num, end_num do
                table.insert(result, i)
            end
        end
    else
        local num = tonumber(part)
        if num then
            table.insert(result, num)
        end
    end
end

if #result > 0 and #result < 14 then
	local s = script:new()
	s.name = "Patrol #"..input
	for _,wp in pairs(result) do
		s:add_action(16, wp)
		s:add_action(5, guard)
	end
	for i=#result-1,2,-1 do
		s:add_action(16, result[i])
		s:add_action(5, guard)
	end
	s:add_action(6, 1)
	s:apply()
	print("���Ѳ�߽ű������ƣ�"..s.name)
	print("ID��"..s.id)
else
	print("·����������٣�")
end