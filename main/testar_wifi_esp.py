import socket

ESP32_IP   = "192.168.15.8"
ESP32_PORT = 12345

comandos = {
    "1": ("SLEEP0",     "Sleep OFF"),
    "2": ("SLEEP1",     "Sleep 10s"),
    "3": ("SLEEP2",     "Sleep 20s"),
    "4": ("SLEEP3",     "Sleep 30s"),
    "5": ("SWING",     "Toggle Swing"),
    "6": ("BUZZER",    "Toggle Buzzer"),
}

def enviar(cmd):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(cmd.encode('ascii'), (ESP32_IP, ESP32_PORT))
    sock.close()

while True:
    print("\n=== Controle ESP32 ===")
    for k, (cmd, desc) in comandos.items():
        print(f"  {k} - {desc}")
    escolha = input("Escolha: ").strip()
    if escolha in comandos:
        cmd, desc = comandos[escolha]
        enviar(cmd)
        print(f"Enviado: {desc}")
    else:
        print("Opção inválida")