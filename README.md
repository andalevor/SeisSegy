# SeisSegy
SEGY r/w library

# Build
You will need SeisTrace library.

git clone --recurse-submodules https://github.com/andalevor/SeisSegy.git

cd SeisSegy

meson setup -Dbuildtype=release build

meson compile -C build

meson install -C build
