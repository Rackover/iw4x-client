iw4of = {
	source = path.join(dependencies.basePath, "iw4-open-formats", "src"),
	dependenciesPath = path.join(dependencies.basePath, "iw4-open-formats", "deps")
}

iw4of_dependencies = {
	basePath = iw4of.dependenciesPath
}
	


function iw4of.import()
	links { "iw4of" }
	
	dir = path.join(dependencies.basePath, "iw4-open-formats", "deps", "premake/*.lua")
	deps = os.matchfiles(dir)
	
	for i, dep in pairs(deps) do
		dep = dep:gsub(".lua", "")
		require(dep)
	end
	
	iw4of.libtomcrypt.import()
	iw4of.libtommath.import()
	iw4of.rapidjson.import()
	iw4of.zlib.import()
	iw4of.zstd.import()
	
	
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
		
		iw4of.libtomcrypt.includes()
		iw4of.libtommath.includes()
		iw4of.rapidjson.includes()
		iw4of.zlib.includes()
		iw4of.zstd.includes()
		
end

table.insert(dependencies, iw4of)
