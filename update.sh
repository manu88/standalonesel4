echo "==> Getting seL4 kernel"
# SeL4 kernel repo
if [ -d "kernel" ]; then
	git -C kernel/ pull
else 
	git clone https://github.com/seL4/seL4.git kernel
fi