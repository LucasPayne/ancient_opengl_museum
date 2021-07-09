# ancient_opengl_museum, April 2020

An "OpenGL museum" created for a computer graphics course. The course required OpenGL 2 to be used in this assignment, so I focused on geometric algorithms rather than shading techniques.
The main features displayed are
    - Rigid body physics ([Eberly: Game Physics](https://www.goodreads.com/book/show/1086297.Game_Physics_With_CDROM_), [Parent: Computer Animation](https://www.elsevier.com/books/computer-animation/parent/978-0-12-415842-9))
    - Collision detection ([Ericson: Real-Time Collision Detection](http://realtimecollisiondetection.net/), [GJK algorithm (video explanation by Casey Muratori)](https://www.youtube.com/watch?v=Qupqu1xe7Io))
    - Convex hull algorithm visualization (using the algorithm described in [O'Rourke: Computational Geometry in C](http://www.science.smith.edu/~jorourke/books/compgeom.html))
    - 3D user interaction (dragging rigidy bodies around, rotating displayed objects, player-environment collision)
    - Surface tessellation (rectangular and triangular B-spline patches, the Utah teapot loaded from its original patch file)
    - Isosurface extraction ([metaballs and marching cubes](http://www.paulbourke.net/geometry/polygonise/))

![pic1](https://github.com/LucasPayne/ancient_opengl_museum/blob/main/report/screenshots/rigid_bodies.png)
![pic2](https://github.com/LucasPayne/ancient_opengl_museum/blob/main/report/screenshots/surfaces.png)
![pic3](https://github.com/LucasPayne/ancient_opengl_museum/blob/main/report/screenshots/convex_hulls.png)
![pic4](https://github.com/LucasPayne/ancient_opengl_museum/blob/main/report/screenshots/museum_from_outside.png)

