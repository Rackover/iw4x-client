iw4_open_formats = {
	source = path.join(dependencies.basePath, "iw4-open-formats/src"),
}

function iw4_open_formats.import()
	links {"iw4-open-formats"}

	iw4_open_formats.includes()
end

function iw4_open_formats.includes()
	includedirs {
		path.join(iw4_open_formats.source, "src/headers")
	}
end

function iw4_open_formats.project()
	project "iw4-open-formats"
		language "C++"

		libtomcrypt.import()
		ibtommath.import()
		rapidjson.import()
		zlib.import()
		iw4_open_formats.includes()

		pchheader "std_include.hpp"
		pchsource (path.join(iw4_open_formats.source, "iw4-of/std_include.cpp"))

		files {
			path.join(iw4_open_formats.source, "iw4-of/**.hpp"),
			path.join(iw4_open_formats.source, "iw4-of/**.cpp").
		}

		includedirs {
			path.join(iw4_open_formats.source, "iw4-of"),
			path.join(dependencies.basePath, "iw4-open-formats", "include")
		}

		warnings "Off"
		kind "StaticLib"
end

table.insert(dependencies, iw4_open_formats)
