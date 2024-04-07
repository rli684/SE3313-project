# Imports
import sys
import socket
import threading
import time
import functools
from PyQt5.QtWidgets import (
    QApplication,
    QMainWindow,
    QSlider,
    QVBoxLayout,
    QWidget,
    QTextEdit,
    QPushButton,
    QLabel,
    QLineEdit,
    QComboBox,
    QMessageBox,
    QDesktopWidget,
)
from PyQt5.QtGui import QIcon
from PyQt5.QtCore import QTimer, pyqtSignal
import select

# Begin class


class ChatRoomGUI(QMainWindow):

    server_shutdown_signal = pyqtSignal()

    # Initialization
    def __init__(self):
        super().__init__()
        self.initUI()
        self.send_disconnect_on_close = True
        self.client_socket = self.connect_to_server()  # Initialize socket connection
        self.running = True
        self.server_shutdown_signal.connect(self.handle_server_shutdown)

    # Slot to handle server shutdown
    def handle_server_shutdown(self):
        QMessageBox.warning(
            self, "Server Shutdown", "The server has been shutdown. Now exiting..."
        )
        if hasattr(self, 'chat_window'):
            self.chat_window.close()
        self.close()

    # GUI settings
    def initUI(self):

        # Title
        self.setWindowTitle("Chatroom Whisperers")

        # WINDOW SIZE
        self.setGeometry(100, 100, 500, 500)

        # Setting window icon
        self.setWindowIcon(
            QIcon("logo.png")
        )  # Replace 'logo.png' with the path to your logo image

        # Creating central widget
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)

        # Layout for children
        layout = QVBoxLayout()  # Arrange in vertical column
        self.central_widget.setLayout(layout)

        # username
        self.username_label = QLabel("Username:")
        self.username_field = QLineEdit()
        layout.addWidget(self.username_label)
        layout.addWidget(self.username_field)

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
        self.create_server_password_label = QLabel(
            "Chatroom Password (Optional):")
        self.create_server_password = QLineEdit()
        self.create_server_password.setEchoMode(QLineEdit.Password)
        layout.addWidget(self.create_server_password_label)
        layout.addWidget(self.create_server_password)

        # Lobby size slider
        self.create_server_lobby_size_label = QLabel("Lobby Size (2-5):")
        self.slider = QSlider()
        self.slider.setOrientation(1)  # Vertical
        self.slider.setMinimum(2)  # Min
        self.slider.setMaximum(5)  # Max
        self.slider.setValue(2)  # Initial value
        self.slider.setTickInterval(1)  # Interval
        self.slider.setTickPosition(QSlider.TicksBelow)  # Tick position
        layout.addWidget(self.create_server_lobby_size_label)
        layout.addWidget(self.slider)

        # Create server button
        self.create_button = QPushButton("Create Chatroom")
        self.create_button.clicked.connect(self.create_button_execute)
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
        self.password_field.setEchoMode(QLineEdit.Password)
        self.password_label.hide()
        self.password_field.hide()
        layout.addWidget(self.password_label)
        layout.addWidget(self.password_field)

        # Connect button
        self.connect_button = QPushButton("Connect")
        self.connect_button.clicked.connect(self.connect_to_room)
        layout.addWidget(self.connect_button)

        # Connect signals
        self.room_combo_box.currentIndexChanged.connect(
            self.update_connect_button_state
        )
        
        centerWindow(self)

    def closeEvent(self, event):
        try:
            if self.client_socket and self.send_disconnect_on_close:
                print("Sending disconnect message...")
                self.client_socket.send("DISCONNECT".encode())
                # Wait for a brief moment to ensure the message is sent
                time.sleep(0.1)
                print("Closing socket...")
                self.client_socket.close()
        except Exception as e:
            print("Error disconnecting from room:", e)
        finally:
            event.accept()

    def start_message_check_timer(self):
        self.message_check_timer = QTimer()
        self.message_check_timer.timeout.connect(self.check_for_messages)
        self.message_check_timer.start(1000)  # Check for messages every second

    def check_for_messages(self):
        try:
            sockets_to_check = [self.client_socket]
            readable, _, _ = select.select(sockets_to_check, [], [], 0)
            for sock in readable:
                if sock == self.client_socket:
                    message = self.client_socket.recv(1024).decode()
                    if message:
                        if (message.split(";")[0] == "UPDATE_DATA"):
                            message = message.split("UPDATE_DATA;")[1].strip()
                            self.populate_room_info(message)
                        if (message == "SERVER_SHUTDOWN"):
                            self.server_shutdown_signal.emit()
        except Exception as e:
            print("Error checking for messages:", e)

    # Create chatroom
    def create_button_execute(self):
        try:
            # Get room information from input fields
            room_name = self.create_server_name.text()
            username = self.username_field.text()
            room_password = self.create_server_password.text()
            lobby_size = self.slider.value()
            if username == "":
                QMessageBox.warning(
                    self, "Invalid Username", "Please enter a username."
                )
                return
            if room_name == "":
                QMessageBox.warning(
                    self, "Invalid Room Name", "Please enter a room name."
                )
                return

            # Send room information to server
            # 1 for the user creating the room
            message = f"CREATE_ROOM;{room_name};{room_password};1;{lobby_size};{username}".encode(
            )
            print("here2")
            self.client_socket.send(message)
            # Now, wait for the server's response
            server_response = self.client_socket.recv(1024).decode()
            print("Server response:", server_response)  # Debugging statement

            if server_response == "CREATE_SUCCESS":
                print("Creating room success")  # Debugging statement
                # If the response is positive, proceed to open the chat window
                self.send_disconnect_on_close = False
                self.close()  # Close the main window
                self.open_chat_window(room_name, username)
                # Start a new thread for message listening
                if(not hasattr(self,"listening")):
                    self.listening = threading.Thread(target=self.receive_messages).start()
                elif(self.running==False):
                    self.running = True
                    self.listening = threading.Thread(target=self.receive_messages).start()
            else:
                # Handle other server responses
                QMessageBox.warning(
                    self,
                    "Connection Error",
                    "Failed to connect to the chat room. Please try again later.",
                )

        except Exception as e:
            print("Error creating chat room:", e)
            QMessageBox.critical(
                self, "Error", "Failed to create chat room. Please try again."
            )

    # Connect to server
    def connect_to_server(self):
        try:
            # Establish a socket connection to the server
            server_address = "127.0.0.1"  # CHANGE THIS TO YOUR SERVER'S IP ADDRESS
            server_port = 3000  # CHANGE THIS TO YOUR SERVER'S PORT
            self.client_socket = socket.socket(
                socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect((server_address, server_port))
            print("Connected to the server.")
            room_data = self.client_socket.recv(1024).decode()
            self.populate_room_info(room_data)
            self.start_message_check_timer()
            return self.client_socket
        except Exception as e:
            print("Error connecting to the server:", e)
            exit(0)

    # Populate available rooms
    def populate_room_info(self, room_data):
        try:
            if (room_data == "NO_ROOMS" or room_data == ""):
                self.available_rooms_label.hide()
                self.join_rooms_label.hide()
                self.room_combo_box.hide()
                self.password_field.hide()
                self.password_field.clear()
                self.password_label.hide()
                return
            self.available_rooms_label.show()
            self.join_rooms_label.show()
            self.room_combo_box.show()

            self.room_combo_box.clear()

            room_data = room_data.splitlines()

            # Add rooms to the combo box
            for line in room_data:
                # Split each line by the delimiter (e.g., comma)
                room_info = line.split(";")

                # Extract values from the split line
                name = room_info[0]
                password = room_info[1]
                current_users = int(room_info[2])
                max_users = int(room_info[3])

                # Add room information to the combo box
                room_display = f"{name} - {'Locked' if password else 'Unlocked'}, {current_users}/{max_users} users"
                self.room_combo_box.addItem(room_display)

            # Check initial room selection
            initial_index = self.room_combo_box.currentIndex()
            self.update_connect_button_state(initial_index)

        except Exception as e:
            print("Error populating room information:", e)

    # Update connect button
    def update_connect_button_state(self, index):
        # Show password field if a locked server is selected
        if index != -1:
            if self.client_socket:
                try:
                    room_data = self.room_combo_box.currentText()
                    is_locked = room_data.split(" - ")[1].startswith("Locked")
                    if is_locked:
                        self.password_label.show()
                        self.password_field.show()
                        # Set window height to default
                        self.setFixedHeight(500)
                    else:
                        self.password_label.hide()
                        self.password_field.hide()
                        self.password_field.clear()
                        # Set window height dynamically
                        self.setFixedHeight(450)
                except Exception as e:
                    print("Error fetching room information:", e)

    # Connecting to room
    def connect_to_room(self):
        selected_index = self.room_combo_box.currentIndex()
        selected_room_info = self.room_combo_box.itemText(selected_index)
        password = self.password_field.text()
        username = self.username_field.text()
        locked = selected_room_info.split(" - ")[1].startswith("Locked")
        if username == "":
            QMessageBox.warning(self, "Invalid Username",
                                "Please enter a username.")
            return

        if password == "" and locked:
            QMessageBox.warning(self, "Invalid Password",
                                "Please enter a password.")
            return

        room_name = selected_room_info.split(" - ")[0]
        print(f"Connecting to: {selected_room_info} with password: {password}")

        if self.client_socket:
            try:
                # Send room name and password to server for validation
                message = f"JOIN_ROOM;{room_name};{password if password else 'NO_PASSWORD'};{username}".encode(
                )
                self.client_socket.send(message)

                # Now, wait for the server's response
                server_response = self.client_socket.recv(1024).decode()
                # Debugging statement
                print("Server response:", server_response)

                if server_response == "JOIN_SUCCESS":
                    print("Joining room success")  # Debugging statement
                    # If the response is positive, proceed to open the chat window
                    self.send_disconnect_on_close = False
                    self.close()  # Close the main window
                    self.open_chat_window(room_name, username)
                    # Start a new thread for message listening
                    if(not hasattr(self,"listening")):
                        self.listening = threading.Thread(target=self.receive_messages).start()
                    elif(self.running==False):
                        self.running = True
                        self.listening = threading.Thread(target=self.receive_messages).start()
                        
                elif server_response == "INVALID_PASSWORD":
                    # Handle invalid password scenario
                    QMessageBox.warning(
                        self,
                        "Invalid Password",
                        "The password you entered is incorrect. Please try again.",
                    )
                elif server_response == "ROOM_FULL":
                    # Handle invalid password scenario
                    QMessageBox.warning(
                        self,
                        "Connection Error",
                        "The room you are joining is full. Please create or join another room.",
                    )
                elif server_response == "NO_ROOM":
                    # Handle invalid password scenario
                    QMessageBox.warning(
                        self,
                        "Connection Error",
                        "There are no rooms available. Create your own room or wait for others to create a room.",
                    )
                elif server_response == "EXISTING_USER":
                    QMessageBox.warning(
                        self,
                        "Connection Error",
                        "This username already exists in this chatroom. Please select a new username.",
                    )
                else:
                    # Handle other server responses
                    QMessageBox.warning(
                        self,
                        "Connection Error",
                        "Failed to connect to the chat room. Please try again later.",
                    )

            except Exception as e:
                print("Error connecting to the room:", e)
                QMessageBox.critical(
                    self,
                    "Error",
                    "Failed to connect to the chat room. Please try again.",
                )

    def open_chat_window(self, room_name, username):
        self.chat_window = ChatWindow(
            room_name, self.client_socket, username, self)
        self.chat_window.show()

    def receive_messages(self):
        while self.running:
            try:
                self.client_socket.settimeout(1)
                message = self.client_socket.recv(
                    1024).decode()  # Receive message from server
                print(message)
                messages = message.splitlines()
                if message:  # Check if message is not empty
                    if (message == "SERVER_SHUTDOWN"):
                        self.server_shutdown_signal.emit()
                    if (len(message.split("MESSAGE;")) == 1):
                        if(len(message.split("UPDATE_DATA;")) > 1):
                            message = message.split("UPDATE_DATA;")[1].strip()
                            self.populate_room_info(message)
                            continue
                        continue
                    message = message.split("MESSAGE;")[1]
                    if (len(message.split("UPDATE_DATA")) > 1):
                        message = message.split("UPDATE_DATA")[0]
                    # Split message into parts
                    parts = message.split(';')
                    # removing leading & trailing spaces
                    username = parts[0].strip()
                    message = parts[1].strip()
                    # Process the received message, update GUI, etc.
                    # Call a method to handle received messages
                    self.process_received_message(f"{username}: {message}")
            except socket.timeout:
                pass
            except Exception as e:
                print(e)
                break  # Break the loop if there's an error or connection is closed

    def process_received_message(self, message):
        # Implement how to handle the received message here
        # For example, display the message in the GUI
        self.chat_window.messages_display.append(message)


class ChatWindow(QWidget):
    def __init__(self, room_name, client_socket, username, parent=None):
        super().__init__()
        self.room_name = room_name
        self.username = username
        self.client_socket = client_socket
        self.parent = parent
        self.initUI()

    def initUI(self):
        self.setWindowIcon(QIcon("logo.png"))
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

        centerWindow(self)
        
    def closeEvent(self, event):
        try:
            if self.client_socket:
                self.parent.running = False
                print("Sending disconnect room message...")
                self.client_socket.send(
                    f"DISCONNECT_ROOM;{self.room_name};{self.username}".encode())
        except Exception as e:
            print("Error disconnecting from room:", e)
        finally:
            self.disconnect_from_room()


    def send_message(self):
        message = self.text_input.text()
        if (message == ""):  # preventing empty messages
            return
        self.messages_display.append(f"{self.username}: {message}")
        message = f"MESSAGE_ROOM;{self.room_name};{message};{self.username}"
        # Placeholder for sending message to server
        print("Sending message:", message)
        self.client_socket.send(message.encode())
        self.text_input.clear()

    def disconnect_from_room(self):
        self.parent.send_disconnect_on_close = True
        self.close()
        self.parent.show()
        self.parent.create_server_name.clear()
        self.parent.create_server_password.clear()
        self.parent.password_field.clear()
        self.parent.slider.setValue(2)
        
def centerWindow(GUI):
    # Get the screen geometry
    screenGeometry = QDesktopWidget().screenGeometry()

    # Calculate the center point of the screen
    x = (screenGeometry.width() - GUI.width()) // 2
    y = (screenGeometry.height() - GUI.height()) // 2

    # Move the window to the center of the screen
    GUI.move(x, y)


def main():
    app = QApplication(sys.argv)
    chatroom = ChatRoomGUI()
    chatroom.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
