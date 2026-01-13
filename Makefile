CC = g++
CFLAGS = -O2 -s -static -mwindows
TARGET = EyeCareReminder.exe
SOURCE = EyeCareReminder.cpp
RESOURCE = resource.rc
RESOURCE_OBJ = resource.o

$(TARGET): $(SOURCE) $(RESOURCE_OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(RESOURCE_OBJ)
	@echo 编译完成！中文版本，无乱码问题

$(RESOURCE_OBJ): $(RESOURCE)
	windres $(RESOURCE) -o $(RESOURCE_OBJ)

clean:
	del $(TARGET) $(RESOURCE_OBJ)

.PHONY: clean