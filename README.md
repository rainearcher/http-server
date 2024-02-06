# C++ HTTP Server

## Project Description

This project is a basic HTTP server, designed to handle requests by either serving local files or proxying remote video files.

## Build Instructions

To build the project, ensure you have `g++` installed on your system. 

- To compile the project:
    ```bash
    make
    ```
  
- To clean the build (remove the compiled output):
    ```bash
    make clean
    ```

## Running the Server

After compiling, you can run the server using the following command:

```bash
./server [-b local_port] [-r remote_host] [-p remote_port]
```

### Command-line Arguments:

- `-b local_port`: Specify the local port on which the HTTP server will run. Defaults to `8081`.
- `-r remote_host`: Specify the remote host's address for proxying. Defaults to `131.179.176.34`.
- `-p remote_port`: Specify the remote port for proxying. Defaults to `5001`.

## Serving local files
The server is able to serve files located in the same directory as server.c with the following extensions:

.html, .txt, .jpg

All other files are served as binary data.

## Proxying
To test the reverse-proxy functionality, startup a backend server with the following steps.

1. Install ffmpeg
2. use this command to convert a .mp4 video file into .ts fiels and .m3u8 file: 
```
ffmpeg -i <videofile>.mp4 -profile:v baseline -level 3.0 -start_number 0
-hls_time 10 -hls_list_size 0 -f hls <manifestfilename>.m3u8
-hls_segment_filename "<tsfilename>%d.ts"
```
3. Place the .m3u8 file into the same directory as the server.c file.
4. Place the .ts files into a separate directory: 
```
mkdir newdir
mv *.ts newdir
```
5. cd to the new directory and run a python command to start up the backend server:
```
cd newdir
python3 -m http.server 5001
```
6. now run the main server as before and navigate to `localhost:8081` to see your video played.