fonts = {}

function fonts.import()
	fonts.includes()
end

function fonts.includes()
	includedirs {
		path.join(dependencies.basePath, "extra/font"),
	}
end

function fonts.project()
	project "fonts"
		language "C"

		fonts.includes()

		files {
			path.join(dependencies.basePath, "extra/fonts/*.hpp"),
		}

		warnings "Off"
		kind "SharedItems"
end

table.insert(dependencies, fonts)
