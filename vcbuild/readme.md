
# Prepare

Install VS 2019.

In a sparate folder, install vcpkg.

```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat

./vcpkg integrate install

./vcpkg install mbedtls:x64-windows mbedtls:x86-windows
./vcpkg install pcre:x64-windows pcre:x86-windows
./vcpkg install zlib:x64-windows zlib:x86-windows

```

Now, you can build the solution whit VS 2019 IDE.
