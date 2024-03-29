# Imports
import sys
import socket
import threading
from PyQt5.QtWidgets import QApplication, QMainWindow, QSlider, QVBoxLayout, QWidget, QTextEdit, QPushButton, QLabel, QLineEdit, QHBoxLayout, QComboBox
from PyQt5.QtGui import QIcon
# Begin class
class ChatRoomGUI(QMainWindow):
    # Initialization
    def __init__(self):
        super().__init__()
        self.initUI()
    # GUI settings
    def initUI(self):
        # Title
        self.setWindowTitle('Chatroom Whisperers')
        # WINDOW SIZE
        self.setGeometry(100, 100, 500, 500)
        # Setting window icon
        self.setWindowIcon(QIcon('logo.png'))  # Replace 'logo.png' with the path to your logo image
        # Creating central widget
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        # Layout for children
        layout = QVBoxLayout()  # Arrange in verticle column
        self.central_widget.setLayout(layout)
        # Label for server creation
        self.create_server_label = QLabel("Create Chatroom")
        self.create_server_label.setStyleSheet("text-decoration: underline;") 
        layout.addWidget(self.create_server_label)
        # Text box for server creation
        self.create_server_name_label = QLabel("Chatroom Name:")
        self.create_server_name = QLineEdit()
        layout.addWidget(self.create_server_name_label)
        layout.addWidget(self.create_server_name)
        # Password box for server creation
        self.create_server_password_label = QLabel("Chatroom Password:")
        self.create_server_password = QLineEdit()
        layout.addWidget(self.create_server_password_label)
        layout.addWidget(self.create_server_password)
        # Lobby size slider
        self.create_server_lobby_size_label = QLabel("Lobby Size (2-5):")
        self.slider = QSlider()
        self.slider.setOrientation(1)  # Verticle
        self.slider.setMinimum(2)  # Min
        self.slider.setMaximum(5)  # Max
        self.slider.setValue(2)  # Initial value
        self.slider.setTickInterval(1)  # Interval
        self.slider.setTickPosition(QSlider.TicksBelow)  # Tick position
        layout.addWidget(self.create_server_lobby_size_label)
        layout.addWidget(self.slider)
        # Create server button
        self.create_button = QPushButton('Create Chatroom')
        self.create_button.clicked.connect(self.create_buttom_execute)
        layout.addWidget(self.create_button)
        # Spacing
        layout.addSpacing(20)  # Add a blank line
        # Join servers
        self.join_rooms_label = QLabel("Join Chat Room")
        self.join_rooms_label.setStyleSheet("text-decoration: underline;")
        layout.addWidget(self.join_rooms_label)
        # Label for available server rooms
        self.available_rooms_label = QLabel("Available Chatrooms:")
        layout.addWidget(self.available_rooms_label)
        # Combo box to select chatroom
        self.room_combo_box = QComboBox()
        layout.addWidget(self.room_combo_box)
        # Password field
        self.password_label = QLabel("Password:")
        self.password_field = QLineEdit()
        self.password_label.hide()
        self.password_field.hide()
        layout.addWidget(self.password_label)
        layout.addWidget(self.password_field)
        # Connect button
        self.connect_button = QPushButton('Connect')
        self.connect_button.clicked.connect(self.connect_to_room)
        layout.addWidget(self.connect_button)
        # Populate room information
        self.populate_room_info()
        # Connect signals
        self.room_combo_box.currentIndexChanged.connect(self.update_connect_button_state)
    # Create chatroom
    def create_buttom_execute(self):
        return
    # Populate available rooms
    def populate_room_info(self):
        # Rooms
        room_data = [
            {"name": "Chris's room", "current_users": 3, "max_users": 4, "locked": True},
            {"name": "Elbert's room", "current_users": 1, "max_users": 5, "locked": False}
        ]
        # Iterate through rooms and add
        for room in room_data:
            locked_status = "Locked" if room["locked"] else "Unlocked"
            room_info = f"{room['name']} - {locked_status}, {room['current_users']}/{room['max_users']} users"
            self.room_combo_box.addItem(room_info)
        # Check initial room selection
        initial_index = self.room_combo_box.currentIndex()
        self.update_connect_button_state(initial_index)
    # Update connect button
    def update_connect_button_state(self, index):
        # Enable connect button if a chatroom is selected
        self.connect_button.setEnabled(index != -1)
        # Room data
        room_data = [
            {"name": "Chris's room", "current_users": 3, "max_users": 4, "locked": True},
            {"name": "Elbert's room", "current_users": 1, "max_users": 5, "locked": False}
        ]
        # Show password field if a locked server is selected
        if index != -1:
            is_locked = room_data[index]["locked"]
            if is_locked:
                self.password_label.show()
                self.password_field.show()
                # Set window height to default
                self.setFixedHeight(500)
            else:
                self.password_label.hide()
                self.password_field.hide()
                # Set window height dynamically
                self.setFixedHeight(450)
    # Connecting to room
    def connect_to_room(self):
        selected_index = self.room_combo_box.currentIndex()
        selected_room_info = self.room_combo_box.itemText(selected_index)
        password = self.password_field.text()  # Retrieve entered password
        room_name = selected_room_info.split(' - ')[0] 
        print(f"Connecting to: {selected_room_info} with password: {password}")
         # Establish a socket connection to the server
        server_address = '127.0.0.1'  # CHANGE THIS TO YOUR SERVERS IP ADDRESS
        server_port = 2200  # CHANGE THIS TO YOUR SERVERS PORT
        
        try:
            client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client_socket.connect((server_address, server_port))
            print("Connected to the server.")
            
            # Close the main window
            self.close()

            # Open the chat window
            self.open_chat_window(room_name,client_socket)
            
             # Start a new thread for message listening
            threading.Thread(target=self.receive_messages, args=(client_socket,)).start()
            
        except Exception as e:
            print("Error connecting to the server:", e)



    def open_chat_window(self, room_name, client_socket):
        self.chat_window = ChatWindow(room_name, client_socket)
        self.chat_window.show()

    def receive_messages(self, client_socket):
        while True:
            try:
                message = client_socket.recv(1024).decode()  # Receive message from server
                if message:  # Check if message is not empty
                    print("Received message:", message)
                    # Process the received message, update GUI, etc.
            except Exception as e:
                print("Error receiving message:", e)
                break  # Break the loop if there's an error or connection is closed
        
class ChatWindow(QWidget):
    def __init__(self, room_name, client_socket):
        super().__init__()
        self.room_name = room_name
        self.client_socket = client_socket
        self.initUI()

    def initUI(self):
        self.setWindowIcon(QIcon('logo.png')) 
        self.setWindowTitle(self.room_name)
        self.setGeometry(100, 100, 400, 400)
        layout = QVBoxLayout()
        self.setLayout(layout)

        # Text display for messages
        self.messages_display = QTextEdit()
        self.messages_display.setReadOnly(True)
        layout.addWidget(self.messages_display)

        # Text input field
        self.text_input = QLineEdit()
        layout.addWidget(self.text_input)

        # Send button
        self.send_button = QPushButton("Send")
        self.send_button.clicked.connect(self.send_message)
        layout.addWidget(self.send_button)

        # Disconnect button
        self.disconnect_button = QPushButton("Disconnect")
        self.disconnect_button.clicked.connect(self.disconnect_from_room)
        layout.addWidget(self.disconnect_button)

    def send_message(self):
        message = self.text_input.text()
        # Placeholder for sending message to server
        print("Sending message:", message)
        self.client_socket.send(message.encode())
        self.text_input.clear()

    def disconnect_from_room(self):
        # Close the chat window
        self.close()

        # Reopen the main window
        main_window = ChatRoomGUI()
        main_window.show()


def main():
    app = QApplication(sys.argv)
    chatroom = ChatRoomGUI()
    chatroom.show()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
