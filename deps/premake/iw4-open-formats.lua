iw4of = {
	source = path.join(dependencies.basePath, "iw4-open-formats", "src"),
	dependenciesPath = path.join(dependencies.basePath, "iw4-open-formats", "deps")
}
	


function iw4of.import()
	links { "iw4of" }
	
	dir = path.join(dependencies.basePath, "iw4-open-formats", "deps", "premake/*.lua")
	deps = os.matchfiles(dir)
	
	-- holy shit
	backup = dependencies.basePath
	dependencies.basePath = iw4of.dependenciesPath
	
	for i, dep in pairs(deps) do
		dep = dep:gsub(".lua", "")
		require(dep)
	end
	
	libtomcrypt.import()
	libtommath.import()
	rapidjson.import()
	zlib.import()
	zstd.import()
	
	
	dependencies.basePath = backup
			
	iw4of.includes()
end

function iw4of.includes()
	includedirs {
		path.join(dependencies.basePath, "iw4-open-formats", "include")
	}
end

function iw4of.project()
	project "iw4of"
		warnings "Off"
		kind "StaticLib"
		language "C++"
		cppdialect "C++latest"

		pchheader "std_include.hpp" -- must be exactly same as used in #include directives
		pchsource (path.join(iw4of.source, "iw4-of/std_include.cpp"))
		
		files {
			path.join(iw4of.source, "iw4-of/**.hpp"),
			path.join(iw4of.source, "iw4-of/**.cpp")
		}
		
		includedirs {
			path.join(iw4of.source, "iw4-of"),
			path.join(dependencies.basePath, "iw4-open-formats", "include")
		}
			
		backup = dependencies.basePath
		dependencies.basePath = iw4of.dependenciesPath
		
		libtomcrypt.includes()
		libtommath.includes()
		rapidjson.includes()
		zlib.includes()
		zstd.includes()
		
		dependencies.basePath = backup
end

table.insert(dependencies, iw4of)
