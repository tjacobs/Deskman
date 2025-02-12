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
   ../servos/SMS_STS.cpp ../servos/SCS.cpp ../servos/SCSerial.cpp \
	-o robot -std=c++11 \
	-I include \
	-I ../src \
	-lwebsockets -Llib \
	-lpv_porcupine -Llib \
	-lasound \
        -lSDL2 -lSDL2_image -lcurl
	#-I /opt/homebrew/Cellar/portaudio/19.7.0/include \
	#-lportaudio -L/opt/homebrew/Cellar/portaudio/19.7.0/lib
