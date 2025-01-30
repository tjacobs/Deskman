# Deskman

The software that runs the deskman robot.


## To build
```
mkdir build
cd build
cmake ..
make
```

## To run
Copy whisper base model [models/ggml-base.en.bin](https://huggingface.co/ggerganov/whisper.cpp/blob/main/ggml-base.bin) into build/models/
```
./stream
```
