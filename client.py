# Imports
import sys, socket, threading, time, select
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
from PyQt5.QtCore import QTimer, pyqtSignal, Qt

# Main window class
class ChatRoomGUI(QMainWindow):

    # Define a signal for server shutdown
    server_shutdown_signal = pyqtSignal()

    # Initialization for main UI
    def __init__(self):

        # Initialize the main UI
        super().__init__()
        self.initUI()
        
        # Flag to send disconnect message on close
        self.send_disconnect_on_close = True
        
        # Initialize client socket connection
        self.client_socket = self.connect_to_server()
        
        # Flag to control thread running state
        self.running = True
        
        # Connect the server shutdown signal to its handler
        self.server_shutdown_signal.connect(self.handle_server_shutdown)

    # Slot to handle server shutdown
    def handle_server_shutdown(self):
        
        # Display a warning message on server shutdown
        QMessageBox.warning(
            self, "Server Shutdown", "The server has been shutdown. Now exiting..."
        )
        
        # Close the chat window if it exists
        if hasattr(self, 'chat_window'):
            self.chat_window.close()
            
        # Close the main window
        self.close()

    # Method to initialize main UI window settings
    def initUI(self):
        
        # Set window title
        self.setWindowTitle("Chatroom Whisperers")
        
        # Set window size
        self.setGeometry(100, 100, 500, 500)
        
        # Set window icon
        self.setWindowIcon(QIcon("logo.png"))
        
        # Creating central widget
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        
        # Layout for children
        layout = QVBoxLayout() 
        self.central_widget.setLayout(layout)
        
        # Username field
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
        self.slider.setOrientation(1)  
        self.slider.setMinimum(2)  
        self.slider.setMaximum(5)  
        self.slider.setValue(2) 
        self.slider.setTickInterval(1)  
        self.slider.setTickPosition(QSlider.TicksBelow) 
        layout.addWidget(self.create_server_lobby_size_label)
        layout.addWidget(self.slider)
        
        # Create server button
        self.create_button = QPushButton("Create Chatroom")
        self.create_button.clicked.connect(self.create_button_execute)
        layout.addWidget(self.create_button)
        
        # Spacing
        layout.addSpacing(20)
        
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
        
        # Center the window
        centerWindow(self)

    # Override keyPressEvent to handle Escape key press
    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Escape:
            self.close()

    # Method to handle window close event
    def closeEvent(self, event):
        try:
            # Client socket exists and we want to send a disconnect message to server
            if self.client_socket and self.send_disconnect_on_close:
                
                # Sending disconnect message
                print("Sending disconnect message...")
                self.client_socket.send("DISCONNECT".encode())
                
                # Wait for a brief moment to ensure the message is sent
                time.sleep(0.1)
                
                # Close socket
                print("Closing socket...")
                self.client_socket.close()
        except Exception as e:
            print("Error disconnecting from room:", e)
        finally:
            event.accept()

    # Method to start the message check timer
    def start_message_check_timer(self):
        
        # Creating and connecting timer
        self.message_check_timer = QTimer()
        self.message_check_timer.timeout.connect(self.check_for_messages)
        
        # Check for data messages every second
        self.message_check_timer.start(1000)  

    # Method to check for incoming data messages
    def check_for_messages(self):
        try:
            # Select content from client socket
            sockets_to_check = [self.client_socket]
            readable, _, _ = select.select(sockets_to_check, [], [], 0)
            
            # Socket has content
            for sock in readable:
                
                # Socket is the client socket
                if sock == self.client_socket:
                    
                    # Recieve and decode message
                    message = self.client_socket.recv(1024).decode()
                    
                    # Message exists
                    if message:
                        
                        # Update data message recieved
                        if (message.split(";")[0] == "UPDATE_DATA"):
                            
                            # Update data
                            message = message.split("UPDATE_DATA;")[1].strip()
                            self.populate_room_info(message)
                            
                        # Server shutdown received
                        elif (message == "SERVER_SHUTDOWN"):
                            
                            # Emit shutdown signal
                            self.server_shutdown_signal.emit()
        except Exception as e:
            print("Error checking for messages:", e)

    # Method to create chatroom
    def create_button_execute(self):
        try:
            # Get room information from input fields
            room_name = self.create_server_name.text()
            username = self.username_field.text()
            room_password = self.create_server_password.text()
            lobby_size = self.slider.value()
            
            # Validate username and roomname are filled in
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
            
            # Send create room message to server
            message = f"CREATE_ROOM;{room_name};{room_password};1;{lobby_size};{username}".encode(
            )
            self.client_socket.send(message)
            
            # Response from server
            server_response = self.client_socket.recv(1024).decode()
            
            # Chatroom creation success
            if server_response == "CREATE_SUCCESS":
                
                # Dont send messages on window close (not disconnecting)
                self.send_disconnect_on_close = False
                
                # Close main window and open chatroom window
                self.close()  
                self.open_chat_window(room_name, username)
                
                # Start a new thread for message listening
                if (not hasattr(self, "listening")):
                    self.listening = threading.Thread(
                        target=self.receive_messages).start()
                elif (self.running == False):
                    self.running = True
                    self.listening = threading.Thread(
                        target=self.receive_messages).start()
            
            # Handle other server responses (connection error)
            else:
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

    # Method to connect to the server
    def connect_to_server(self):
        try:
            # Establish a socket connection to the server
            server_address = "3.89.83.172"  
            server_port = 3000  
            self.client_socket = socket.socket(
                socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect((server_address, server_port))
            print("Connected to the server.")
            
            # Decode room data on server connection
            room_data = self.client_socket.recv(1024).decode()
            
            # Populate room data and start timer for dynamic update message checking
            self.populate_room_info(room_data)
            self.start_message_check_timer()
            
            # Return socket object
            return self.client_socket
        except Exception as e:
            print("Error connecting to the server:", e)
            exit(0)

    # Method to populate available rooms in the GUI
    def populate_room_info(self, room_data):
        try:
            # No current rooms, hide join existing room information
            if (room_data == "NO_ROOMS" or room_data == ""):
                self.available_rooms_label.hide()
                self.join_rooms_label.hide()
                self.room_combo_box.hide()
                self.password_field.hide()
                self.password_field.clear()
                self.password_label.hide()
                return
            
            # Show join room data
            self.available_rooms_label.show()
            self.join_rooms_label.show()
            self.room_combo_box.show()
            self.room_combo_box.clear()
            room_data = room_data.splitlines()
            
            # Add rooms to the combo box
            for line in room_data:
                
                # Split each line by the delimiter
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

    # Method to update connect button state based on room selection
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
                        self.setFixedHeight(500)
                    else:
                        self.password_label.hide()
                        self.password_field.hide()
                        self.password_field.clear()
                        self.setFixedHeight(450)
                except Exception as e:
                    print("Error fetching room information:", e)

    # Method to connect to a room
    def connect_to_room(self):
        
        # Getting selected combo box room data
        selected_index = self.room_combo_box.currentIndex()
        selected_room_info = self.room_combo_box.itemText(selected_index)
        password = self.password_field.text()
        username = self.username_field.text()
        locked = selected_room_info.split(" - ")[1].startswith("Locked")
        room_name = selected_room_info.split(" - ")[0]
        
        # Validate username and password are filled
        if username == "":
            QMessageBox.warning(self, "Invalid Username",
                                "Please enter a username.")
            return
        if password == "" and locked:
            QMessageBox.warning(self, "Invalid Password",
                                "Please enter a password.")
            return
        print(f"Connecting to: {selected_room_info} with password: {password}")
        
        # Client socket is connected
        if self.client_socket:
            try:
                # Send room name and password to server for validation
                message = f"JOIN_ROOM;{room_name};{password if password else 'NO_PASSWORD'};{username}".encode(
                )
                self.client_socket.send(message)
                
                # Wait for server response
                server_response = self.client_socket.recv(1024).decode()
                
                # Join chatroom success
                if server_response == "JOIN_SUCCESS":
                    
                    # Dont send messages on window close (not disconnecting)
                    self.send_disconnect_on_close = False
                    
                    # Close main GUI and open chatroom GUI
                    self.close() 
                    self.open_chat_window(room_name, username)
                    
                    # Start a new thread for message listening
                    if (not hasattr(self, "listening")):
                        self.listening = threading.Thread(
                            target=self.receive_messages).start()
                    elif (self.running == False):
                        self.running = True
                        self.listening = threading.Thread(
                            target=self.receive_messages).start()
                        
                # Handle invalid password scenario
                elif server_response == "INVALID_PASSWORD":
                    QMessageBox.warning(
                        self,
                        "Invalid Password",
                        "The password you entered is incorrect. Please try again.",
                    )
                    
                # Handle room full scenario
                elif server_response == "ROOM_FULL":
                    QMessageBox.warning(
                        self,
                        "Connection Error",
                        "The room you are joining is full. Please create or join another room.",
                    )
                    
                # Handle no room scenario
                elif server_response == "NO_ROOM":
                    QMessageBox.warning(
                        self,
                        "Connection Error",
                        "There are no rooms available. Create your own room or wait for others to create a room.",
                    )
                    
                # Handle existing username  scenario
                elif server_response == "EXISTING_USER":
                    QMessageBox.warning(
                        self,
                        "Connection Error",
                        "This username already exists in this chatroom. Please select a new username.",
                    )
                    
                # Handle other server responses
                else:
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

    # Method to open the chat window
    def open_chat_window(self, room_name, username):
        self.chat_window = ChatWindow(
            room_name, self.client_socket, username, self)
        self.chat_window.show()

    # Method to receive messages from chatroom
    def receive_messages(self):
        while self.running:
            try:
                # Receive message from server
                self.client_socket.settimeout(1)
                message = self.client_socket.recv(
                    1024).decode() 
                
                # Message exists
                if message: 
                    
                    # Server shutdown, emit shutdown signal
                    if (message == "SERVER_SHUTDOWN"):
                        self.server_shutdown_signal.emit()
                        
                    # Update message recieved on leaving, update room data
                    if (len(message.split("MESSAGE;")) == 1):
                        if (len(message.split("UPDATE_DATA;")) > 1):
                            message = message.split("UPDATE_DATA;")[1].strip()
                            self.populate_room_info(message)
                            continue
                        continue
                    
                    # Extract message from server message
                    message = message.split("MESSAGE;")[1]
                    if (len(message.split("UPDATE_DATA")) > 1):
                        message = message.split("UPDATE_DATA")[0]
                        
                    # Split message into parts
                    parts = message.split(';')
                    
                    # Removing leading & trailing spaces
                    username = parts[0].strip()
                    message = parts[1].strip()
                    
                    # Call a method to handle received messages
                    self.process_received_message(f"{username}: {message}")
            except socket.timeout:
                pass
            except Exception as e:
                print(e)
                break

    # Method to process received messages
    def process_received_message(self, message):
        self.chat_window.messages_display.append(message)


# Class for Chat Window
class ChatWindow(QWidget):
    def __init__(self, room_name, client_socket, username, parent=None):
        super().__init__()
        self.room_name = room_name
        self.username = username
        self.client_socket = client_socket
        self.parent = parent
        self.initUI()

    # Initialize the UI for chat window
    def initUI(self):
        
        # Set up window icon
        self.setWindowIcon(QIcon("logo.png"))
        
        # Set up window title
        self.setWindowTitle(self.room_name)
        
        # Set up window geometry
        self.setGeometry(100, 100, 450, 500)
        
        # Set up child layout
        layout = QVBoxLayout()
        self.setLayout(layout)

        # Text display for messages
        self.messages_display = QTextEdit()
        self.messages_display.setReadOnly(True)
        layout.addWidget(self.messages_display)

        # Text input field
        self.text_input = QLineEdit()
        self.text_input.returnPressed.connect(self.send_message) 
        layout.addWidget(self.text_input)

        # Send button
        self.send_button = QPushButton("Send")
        self.send_button.clicked.connect(self.send_message)
        layout.addWidget(self.send_button)

        # Disconnect button
        self.disconnect_button = QPushButton("Disconnect")
        self.disconnect_button.clicked.connect(self.disconnect_from_room)
        layout.addWidget(self.disconnect_button)

        # Center window
        centerWindow(self)

    # Override keyPressEvent to handle Escape key press
    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Escape:
            self.disconnect_from_room()

    # Method to handle window close event
    def closeEvent(self, event):
        try:
            # Socket is connected
            if self.client_socket:
                
                # Stop loop for messages
                self.parent.running = False
                
                # Send disconnect message
                print("Sending disconnect room message...")
                self.client_socket.send(
                    f"DISCONNECT_ROOM;{self.room_name};{self.username}".encode())
        except Exception as e:
            print("Error disconnecting from room:", e)
        finally:
            self.disconnect_from_room()

    # Method to send message to the room
    def send_message(self):
        
        # Get message from input field
        message = self.text_input.text()
        
        # Preventing empty messages
        if (message == ""):  
            return
        
        # Append message to screen
        self.messages_display.append(f"{self.username}: {message}")
        
        # Send message to server for chatroom
        message = f"MESSAGE_ROOM;{self.room_name};{message};{self.username}"
        print("Sending message:", message)
        self.client_socket.send(message.encode())
        self.text_input.clear()

    # Method to disconnect from the room
    def disconnect_from_room(self):
        self.parent.send_disconnect_on_close = True
        self.close()
        self.parent.show()
        self.parent.create_server_name.clear()
        self.parent.create_server_password.clear()
        self.parent.password_field.clear()
        self.parent.slider.setValue(2)

# Function to center the window on the screen
def centerWindow(GUI):
    
    # Get the screen geometry
    screenGeometry = QDesktopWidget().screenGeometry()
    
    # Calculate the center point of the screen
    x = (screenGeometry.width() - GUI.width()) // 2
    y = (screenGeometry.height() - GUI.height()) // 2
    
    # Move the window to the center of the screen
    GUI.move(x, y)

# Main function
def main():
    app = QApplication(sys.argv)
    chatroom = ChatRoomGUI()
    chatroom.show()
    sys.exit(app.exec_())

# Entry point of the program
if __name__ == "__main__":
    main()
