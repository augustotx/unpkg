local pkg = {}
local cache = os.getenv("UNPKG_CACHE")

pkg.name = "st-augustotx"
pkg.srcType = "git"
pkg.repo = "https://github.com/augustotx/st"

function pkg.compile()
	os.execute(
		"git clone "
			.. pkg.repo
			.. " "
			.. cache
			.. "/"
			.. pkg.name
			.. "&& cd "
			.. cache
			.. "/"
			.. pkg.name
			.. "&& make st"
			.. "&& mkdir -p build/bin build/usr/share/terminfo"
			.. "&& mv st build/bin"
			.. "&& tic st.info -o build/usr/share/terminfo -x"
	)
end

pkg.outDir = cache .. "/" .. pkg.name .. "/build"
pkg.deps = { "xorg", "freetype", "fontconfig" }
pkg.conflicts = { "st" }
pkg.commands = { "st" }

return pkg
