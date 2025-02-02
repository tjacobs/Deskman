g++ speak.cpp -o speak -std=c++11 \
	-I include \
	-I /opt/homebrew/Cellar/portaudio/19.7.0/include \
	-lportaudio -L/opt/homebrew/Cellar/portaudio/19.7.0/lib \
	-lpv_porcupine -Llib \
	-lwebsockets -Llib 

