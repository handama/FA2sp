local function clone(tbl)
    local new = {}
    for k, v in pairs(tbl) do
        new[k] = v
    end
    return new
end

mode = message_box([[Phobos B49+ 修改了触发行为[125 将建筑建于...]的默认值。
ini中的第三个参数在Phobos 0.3中表示是\否播放建造动画，
但在Phobos B49+中是反过来的。
如果你的地图曾经指定过这个值，在Phobos B49+中需要修改。

点击“是”将该参数全部设为0（播放）
点击“否”将该参数全部设为1（不播放）
点击“取消”将该参数取反]], "选择模式", 3)
create_snapshot()
for i,id in pairs(get_keys("Triggers")) do
	local trigger = get_trigger(id)
	local actions = clone(trigger.actions)
	for i,action in pairs (actions) do
		local params = split_string(action)
		if params[1] == "125" then
			print("已修改触发: "..id..", 行为: "..i-1)
			if mode == 1 then
				params[4] = "0"
			elseif mode == 2 then
				params[4] = "1"
			else
				if params[4] == "0" then
					params[4] = "1"
				else
					params[4] = "0"
				end
			end
		end
		actions[i] = table.concat(params, ",")
	end
	for i = #actions,1,-1 do
		trigger:delete_action(i)
	end
	for i = 1,#actions do
		trigger:add_action(actions[i])
	end
	trigger:apply()
end
update_trigger()