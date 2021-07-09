# Note:
#     Use the script ./run in the root
#     project directory to run the program after forcing a recompilation.
#
#     Header dependencies are important and not included here.

CC=gcc -Iinclude
CFLAGS=-lglut -lGL -lGLU -lm

run: museum
	./museum

clean:
	rm -rf build/*

build/_museum.o: src/museum.c
	$(CC) -o $@ -c src/museum.c $(CFLAGS)
build/mathematics.o: src/mathematics.c
	$(CC) -o $@ -c  src/mathematics.c $(CFLAGS)
build/doubly_linked_list.o: src/doubly_linked_list.c
	$(CC) -o $@ -c  src/doubly_linked_list.c $(CFLAGS)
build/entities.o: src/entities.c
	$(CC) -o $@ -c  src/entities.c $(CFLAGS)
build/input.o: src/input.c
	$(CC) -o $@ -c  src/input.c $(CFLAGS)
build/geometry.o: src/geometry.c
	$(CC) -o $@ -c  src/geometry.c $(CFLAGS)
build/collision.o: src/collision.c
	$(CC) -o $@ -c  src/collision.c $(CFLAGS)
build/camera.o: src/camera.c
	$(CC) -o $@ -c  src/camera.c $(CFLAGS)
build/control_widget.o: src/control_widget.c
	$(CC) -o $@ -c  src/control_widget.c $(CFLAGS)
build/rendering.o: src/rendering.c
	$(CC) -o $@ -c  src/rendering.c $(CFLAGS)
build/player.o: src/player.c
	$(CC) -o $@ -c  src/player.c $(CFLAGS)
build/textures.o: src/textures.c
	$(CC) -o $@ -c  src/textures.c $(CFLAGS)
build/models.o: src/models.c
	$(CC) -o $@ -c  src/models.c $(CFLAGS)
build/trackball.o: src/trackball.c
	$(CC) -o $@ -c  src/trackball.c $(CFLAGS)

build/Exhibits/Exhibit_convex_hull.o: src/Exhibits/Exhibit_convex_hull.c
	$(CC) -o $@ -c  src/Exhibits/Exhibit_convex_hull.c $(CFLAGS)
build/Exhibits/Exhibit_rigid_body_dynamics.o: src/Exhibits/Exhibit_rigid_body_dynamics.c
	$(CC) -o $@ -c  src/Exhibits/Exhibit_rigid_body_dynamics.c $(CFLAGS)
build/Exhibits/Exhibit_curves_and_surfaces.o: src/Exhibits/Exhibit_curves_and_surfaces.c
	$(CC) -o $@ -c  src/Exhibits/Exhibit_curves_and_surfaces.c $(CFLAGS)
build/Exhibits/Exhibit_interactions.o: src/Exhibits/Exhibit_interactions.c
	$(CC) -o $@ -c  src/Exhibits/Exhibit_interactions.c $(CFLAGS)

museum: build/_museum.o build/mathematics.o build/doubly_linked_list.o build/entities.o build/input.o build/geometry.o build/collision.o build/camera.o build/control_widget.o build/trackball.o build/rendering.o build/player.o build/textures.o build/models.o build/Exhibits/Exhibit_convex_hull.o build/Exhibits/Exhibit_rigid_body_dynamics.o build/Exhibits/Exhibit_curves_and_surfaces.o build/Exhibits/Exhibit_interactions.o
	$(CC) -o museum $^ $(CFLAGS)

code_generation: build/mathematics.o build/doubly_linked_list.o build/geometry.o
	$(CC) -o code_generation src/code_generation.c $^ $(CFLAGS)
