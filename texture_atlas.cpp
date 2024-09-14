#include <dirent.h>

namespace TextureAtlas {
    #define WIDTH_PER_SPRITE 32
    void createTextureAtlas(char *folder) {
        char *imgFileTypes[] = {"jpg", "jpeg", "png", "bmp", "PNG"};
        FileNameOfType files = getDirectoryFilesOfType(folder, imgFileTypes, arrayCount(imgFileTypes));

        float outputW = 4096;
        float outputH = 4096;

        float xAt = 0;
        float yAt = 0;

        for(int i = 0; i < files.count; ++i) {
            char *name = files.names[i];
            
            Texture t = loadTextureToGPU(name);

            //NOTE: Draw the texture at the coord
            

            xAt += WIDTH_PER_SPRITE;

            if(xAt >= outputW) {
                xAt = 0;
                yAt =+ WIDTH_PER_SPRITE;
            }

            if(yAt >= outputH) {
                assert(false);
                //NOTE: Out of room
            }
        }
    }
};
