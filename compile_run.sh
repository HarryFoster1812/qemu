BINARY=$1

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 path/to/binary"
    exit 1
fi

# 1. Build the project
# Using ninja directly is often faster if meson is already configured
ninja -C build 

# 2. Kill any old QEMU instances to free up the SDL window and GDB port
pkill qemu-system-riscv32 2>/dev/null

# 3. Launch QEMU
# - Removed -nographic so the SDL window can pop up
# - Added -serial stdio so you can still use printf in your C code
# - -D qemu.log -d cpu,in_asm helps you debug if the LED doesn't blink
./build/qemu-system-riscv32 \
    -machine comp22712 \
    -serial stdio \
    -device loader,file="$BINARY",addr=0x0,cpu-num=0 \
    -D qemu.log \
    -d cpu,in_asm \
    -s -S &

# 4. Give QEMU a second to initialize the GDB server
sleep 1

# 5. Automatically launch GDB and connect
gdb "$BINARY" -ex "target remote :1234" -ex "layout asm" -ex "layout regs"
