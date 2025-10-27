import socket
import threading
import time
import argparse
from collections import defaultdict
import select
import os
from dataclasses import dataclass
from typing import Optional, List
import queue

@dataclass
class Node:
    
    """
    Represents a node in the multi-linked list structure.
    Each node contains a line of text and multiple pointers for different traversal paths.
    """
    content: str
    next: Optional['Node'] = None
    book_next: Optional['Node']= None
    next_frequent_search: Optional['Node']= None
    contains_pattern: bool= False

class SharedList:
    def __init__(self):
        self.head =None
        self.tail= None
        self.book_heads= {}
        self.book_tails={}
        self.lock=threading.Lock()
        
    def add_node(self, content: str, book_id: int) -> Node:      
        
        with self.lock:
            new_node = Node(content)
            if not self.head:
                self.head = new_node
            else:
                self.tail.next = new_node
            self.tail = new_node
            if book_id not in self.book_heads:
                self.book_heads[book_id]= new_node
                self.book_tails[book_id]= new_node
            else:
                self.book_tails[book_id].book_next= new_node
                self.book_tails[book_id] =new_node
            return new_node

class Server:
    def __init__(self, port: int, pattern: str):
        self.port=port
        self.pattern= pattern.lower()
        self.shared_list= SharedList()
        self.connection_count = 0
        self.connection_lock= threading.Lock()
        self.pattern_frequency= defaultdict(int)
        self.pattern_lock= threading.Lock()
        self.running= True
        self.last_analysis_time= 0
        self.analysis_interval= 5
        
    def start(self):
        for _ in range(2):
            analysis_thread=threading.Thread(target=self.pattern_analysis)
            analysis_thread.daemon=True
            analysis_thread.start()
        server_socket=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind(('', self.port))
        server_socket.listen(10)
        print(f"Server listening on port {self.port}")
        while True:
            client_socket,addr= server_socket.accept()
            with self.connection_lock:
                self.connection_count+= 1
                current_count =self.connection_count
            client_thread= threading.Thread(
                target=self.handle_client,
                args=(client_socket, current_count)
            )
            client_thread.daemon= True
            client_thread.start()

    def handle_client(self, client_socket: socket.socket, connection_id: int):
        print(f"New connection {connection_id} established")
        client_socket.setblocking(False)
        buffer =""
        first_line= True
        book_title= ""
        book_content= []
        while True:
            try:
                ready = select.select([client_socket], [], [], 0.1)
                if ready[0]:
                    data=client_socket.recv(4096).decode('utf-8')
                    if not data:
                        break
                    buffer += data
                    while '\n' in buffer:
                        line,buffer= buffer.split('\n', 1)
                        line= line.strip()
                        
                        if first_line:
                            book_title = line
                            first_line = False
                            print(f"Received book: {book_title}")
                        node =self.shared_list.add_node(line, connection_id)
                        print(f"Added node: {line[:50]}...")
                        book_content.append(line)
                        if self.pattern in line.lower():
                            node.contains_pattern = True
                            with self.pattern_lock:
                                self.pattern_frequency[book_title] += 1
                
            except socket.error:
                continue
        filename= f"book_{connection_id:02d}.txt"
        with open(filename, 'w', encoding='utf-8') as f:
            f.write('\n'.join(book_content))
        print(f"Wrote {filename}")
        
        client_socket.close()

    def pattern_analysis(self):
        while self.running:
            current_time= time.time()
            if current_time - self.last_analysis_time >= self.analysis_interval:
                with self.pattern_lock:
                    if current_time - self.last_analysis_time >= self.analysis_interval:
                        self.last_analysis_time = current_time
                        sorted_books = sorted(
                            self.pattern_frequency.items(),
                            key=lambda x: x[1],
                            reverse=True
                        )
                        
                        print("\nPattern Analysis Results:")
                        print(f"Search pattern: '{self.pattern}'")
                        print("Books sorted by pattern frequency:")
                        for book, freq in sorted_books:
                            print(f"- {book}: {freq} occurrences")
                        print()
            
            time.sleep(0.1)

def main():
    parser = argparse.ArgumentParser(description='Multi-threaded Network Server')
    parser.add_argument('-l', '--port', type=int, required=True,
                      help='Port number to listen on')
    parser.add_argument('-p', '--pattern', type=str, required=True,
                      help='Search pattern to analyze')
    
    args = parser.parse_args()
    server = Server(args.port, args.pattern)
    try:
        server.start()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        server.running = False

if __name__ == "__main__":
    main()