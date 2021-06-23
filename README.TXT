Hazed Mirror Productions (http://hazedmirror.gmdude66.net/index.php) presents
The Easy Accelerated Image library v. 0.2


_INTRODUCTION_

Most new C/C++ PSP developers use graphics.c/h for rendering images. However, in my own experience, the graphics.c/h library is too slow and lacks image rotation and scaling, but it is very easy to use. So I thought of making a library that is as easy but renders images using Vertices in ortho projection mode instead of using GU_SPRITES, so that faster render speed, rotation, and scaling could be supported.

This release is marked as version 0.2. It is a major improvement over 0.1, as it adds many bug fixes. In fact, it is probably usable enough to create entertaining homebrew that runs at over 60 frames per second.

Please report any bugs or feel free to fix them and re-release (see LICENSE.TXT).


_CHANGES_

 - Fixed idiotic dlist declaration (was supposed to be a char* not u32*)
 - Fixed drawPartImage. It seems to actually work now.
 - Fixed whatever bug was causing all the draw functions to fail randomly.
 - Fixed the function printTextScreen. It now actually works as it should.
 - Added function drawImageToImage.
 - Added function printTextImage.
 - Added function unswizzleImage. All functions whose height is evenly divisible by 8 are swizzled when created. One can use this function to unswizzle them (but I can't think of a good reason to do that.)
 - Package has been reduced to only two files to reduce clutter: graphics2d.h and graphics2d.c.
 - Lots of other stuff I don't feel like listing.


_KNOWN_BUGS_

 - The functions setImagePixelAt() and drawImageToImage() do not work together too well. Do not apply them to the same image.
 - I'm sure there are other problems just waiting to appear unexpectedly.


_NEXT_VERSION_

 - On the next release, 0.3, I will add functions such as drawLine, saveScreen, and fix the strange problem that is causing trouble between drawImageToImage and setImagePixelAt.


_TIPS_

 - All rendering function calls (such as printTextScreen, drawImage, and drawPartImage) go between the function calls "startDrawing()" and "endDrawing." The call to "flipScreen()" should go directly after "endDrawing()."
 - Do not apply the functions "drawImageToImage" and "setImagePixelAt" to the same image, otherwise, the display will get messed up and show weird colors. I have no idea what causes this since I tried to do these functions together during initialization not in the main loop.
 - See the example demo for more details on how to use the functions.

See the source files (graphics2d.c/h) and the demo (main.c) for more details about specific functions.


_LICENSE_

This library is released under the FreeBSD license. See LICENSE.TXT for details.

Anyone can modify and re-release the source code or use it freely in closed-source applications. I don't care if anyone asks for permission or not; I made this library to improve the quality of the C/C++ homebrew development community not for scene credit. I release games, not libraries, whenever I want fame in the community.


SPECIAL THANKS to

 - My development team Hazed Mirror Productions. (Actually I wrote the source code by myself since the Hazed Mirror Forums are down.)
 - The developers at #psp-programming, who helped me put the right values in all those sceGu* function calls.


