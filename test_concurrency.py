import threading
import time
from test_client import run_test

def run_concurrent_test():
    NUM_CLIENTS = 5
    threads = []
    
    # Avvia più client in parallelo
    for _ in range(NUM_CLIENTS):
        t = threading.Thread(target=run_test)
        threads.append(t)
        t.start()
        time.sleep(0.1)  # Piccolo ritardo per evitare sovraccarico
    
    # Attendi il completamento
    for t in threads:
        t.join()
    
    print(f"Test concorrenza completato con {NUM_CLIENTS} client simultanei")

if __name__ == '__main__':
    run_concurrent_test()