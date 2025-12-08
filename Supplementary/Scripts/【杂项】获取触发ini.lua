local load_from = "map"
local sections = {"Triggers", "Tags", "Actions", "Events"}
local output = "触发INI：\n"
for i,section in ipairs(sections) do
	output = output.."\n["..section.."]\n"
	local kvps = get_key_value_pairs(section, load_from)
	for k,v in pairs(kvps) do
		output = output..k.."="..v.."\n"
	end
end
print(output)
