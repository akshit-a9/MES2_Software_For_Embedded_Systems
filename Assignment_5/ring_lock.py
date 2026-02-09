import threading
import argparse


def main():
    parser = argparse.ArgumentParser(description="DragonBoard Ping-Pong Simulation with Lock")
    parser.add_argument("--buffer", type=int, default=5, help="Size of the buffer")
    parser.add_argument("--items", type=int, default=10, help="Total items to process")
    args = parser.parse_args()

    buffer = [None] * args.buffer
    prod_head = 0
    cons_tail = -1
    lock = threading.Lock()

    def producer():
        nonlocal prod_head, cons_tail

        item_id = 0
        while item_id < args.items:
            with lock:
                next_head = (prod_head + 1) % args.buffer
                # Buffer not full when head is not one step behind tail.
                if next_head != (cons_tail + 1) % args.buffer:
                    buffer[prod_head] = f"Item-{item_id}"
                    print(f"Produced: {buffer[prod_head]} at position {prod_head}")
                    prod_head = next_head
                    item_id += 1

    def consumer():
        nonlocal prod_head, cons_tail

        consumed = 0
        while consumed < args.items:
            with lock:
                next_tail = (cons_tail + 1) % args.buffer
                # Buffer not empty when tail isn't aligned with head.
                if next_tail != prod_head:
                    print(f"     Consumed: {buffer[next_tail]} from position {next_tail}")
                    cons_tail = next_tail
                    consumed += 1

    prod_thread = threading.Thread(target=producer)
    cons_thread = threading.Thread(target=consumer)
    prod_thread.start()
    cons_thread.start()
    prod_thread.join()
    cons_thread.join()


if __name__ == "__main__":
    main()
