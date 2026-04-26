import socket

ESP32_IP   = "192.168.15.9"  # ← IP que aparece no log da ESP32
ESP32_PORT = 12345

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(b"SLEEP", (ESP32_IP, ESP32_PORT))
sock.close()

print("Pacote SLEEP enviado!")