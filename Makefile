# 需要的.o文件
objs := main.o

Crucis : $(objs)
	arm-linux-gnueabihf-gcc -o Crucis $^

# 需要判断是否存在依赖文件
# .main.o.d
dep_files := $(foreach f, $(objs), .$(f).d)
dep_files := $(wildcard $(dep_files))

# 把依赖文件包含进来
ifneq ($(dep_files),)
	include $(dep_files)
endif

%.o : %.c
	arm-linux-gnueabihf-gcc -Wp,-MD,.$@.d -c -o $@ $<

clean:
	rm *.o Crucis -f

distclean:
	rm $(dep_files) *.o Crucis -f