# Run "source build.sh" to set this
export LD_LIBRARY_PATH=./lib
export DISPLAY=:0

# Build
g++ \
   main.cpp \
   speak.cpp \
   screen.cpp \
   face.cpp \
   servos.cpp \
   servos/SMS_STS.cpp servos/SCS.cpp servos/SCSerial.cpp \
	-o robot -std=c++11 \
	-I include \
	-I ../src \
	-lwebsockets -Llib \
	-lpv_porcupine -Llib \
        -lSDL2 -lSDL2_image -lcurl \
        -I /opt/homebrew/Cellar/sdl2/2.30.9/include/SDL2 \
        -I /opt/homebrew/Cellar/sdl2_gfx/1.0.4/include \
	-I /opt/homebrew/Cellar/portaudio/19.7.0/include \
	-lSDL2_gfx -L/opt/homebrew/Cellar/sdl2_gfx/1.0.4/lib \
	-lportaudio -L/opt/homebrew/Cellar/portaudio/19.7.0/lib
