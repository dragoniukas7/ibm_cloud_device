struct memory {
	long totalMemory;
	long freeMemory;
	long sharedMemory;
	long bufferedMemory;
};

int get_usage(struct memory *mem);
