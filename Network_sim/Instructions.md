- Instructions to build image
```bash
docker build -t custom-ns3 .
```
- Instuctions to run and start the container
```bash
docker run -it --rm -v <Path to this directory>\ns3-projects:/usr/ns3/scratch/my-projects custom-ns3
```


```bash
docker run -it --rm custom-ns3
```
```bash
docker exec -it 2f946bd0d93f bash
```

- Instructions for installing 
```bash
apt-get update && apt-get install -y vim
```

- Instructions for ns-3
```bash
./ns3 run first
./ns3 run second
./ns3 run third
./ns3 run fourth
./ns3 run fifth
./ns3 run hello-simulator
``` 

```bash
mv *.cwnd *.pcap *.tr scratch/my-projects/
```

- Instructions for TCP variants (cwnd)
```bash
./ns3 run "scratch/my-projects/tcp-cwnd-variants --tcpVariant=TcpCubic"
./ns3 run "scratch/my-projects/tcp-cwnd-variants --tcpVariant=TcpBic"
./ns3 run "scratch/my-projects/tcp-cwnd-variants --tcpVariant=TcpNewReno"
```