local shipYards = split_string(get_string("General", "Shipyard", "GAYARD,NAYARD,YAYARD", "rules+map"))
for i,yard in pairs(shipYards) do
	if yard ~= "" then
		write_string(yard, "TechLevel", "11")
	end
end