CPPFLAGS=-Wall -g3 \
 				`pkg-config glib-2.0 --cflags`\
 				`pkg-config gobject-2.0 --cflags`\
				`pkg-config json-glib-1.0 --cflags`
#				-I$(HOME)/work/anerian/services/workq/json/json-glib-0.6.2/\
#				-I$(HOME)/work/anerian/services/workq/json/json-glib-0.6.2/json-glib

LDFLAGS=-lstdc++ -lpthread -lev \
			  `pkg-config glib-2.0 --libs`\
				`pkg-config gobject-2.0 --libs`\
				`pkg-config json-glib-1.0 --libs`
				
#        -L$(HOME)/work/anerian/services/workq/json/json-glib-0.6.2/json-glib/.libs -ljson-glib-1.0


all: json obj
	./json sample.json
	./obj

obj: obj.o
json: json.o

clean:
	rm -f json json.o obj obj.o
