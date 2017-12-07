APP = ia6270-testapp

# Add any other object files to this list below
APP_OBJS = ia6270-testapp.o

all: build

build: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

