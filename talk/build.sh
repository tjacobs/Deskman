g++ speak.cpp -o speak -std=c++11 \
	-I include \
	-lwebsockets -Llib \
	-lpv_porcupine -Llib \
	-lasound
	#-I /opt/homebrew/Cellar/portaudio/19.7.0/include \
	#-lportaudio -L/opt/homebrew/Cellar/portaudio/19.7.0/lib

# Run "source build.sh" to set this
export LD_LIBRARY_PATH=./lib
