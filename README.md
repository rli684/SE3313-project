# Chatroom Whisperers

Welcome to Chatroom Whisperers! This is a simple chatroom application built using C++ and Python. With Chatroom Whisperers, you can connect with others and engage in conversations in real-time.

## Installation

To run the chatroom, you have two options:

### Option 1: Clone the Repository

1. Clone the repository to your local machine.
2. Install Python if you haven't already.
3. Install the Python dependency PyQt5. You can do this using pip:

   ```bash
   pip install PyQt5
   ```
4. Run the ```client.py``` file using python. You can do this by running:
   
   ```bash
   python client.py
   ```

### Option 2: Download the Executable
1. Download the executable from the GitHub repository.
2. Open Command Prompt in Windows.
3. Navigate to the directory containing the executable.
4. Run the executable using the following command:

   ```bash
   cmd /K client.exe
   ```

Both options will connect you to the Chatroom Whisperers server hosted at ```54.163.37.13``` on port ```2004```, which is hosted on an AWS service.

## Usage

Once you've connected to the server, you can start chatting with other users who are also connected. Here are some additional functionalities:

- **Create/Join Rooms:** You can create your own chat rooms or join existing ones. Simply enter the room name or select from the list of available rooms.

- **Set Up Max Lobby Size:** As a room owner, you can set up the maximum number of users allowed in the room (lobby size). This helps in managing the conversation flow and ensuring a better chat experience.

- **Make Rooms Private:** If you want to restrict access to your room, you can make it private. Only users with the invitation or the room password can join.

- **Customize Username/Server:** You have the option to customize your username and server preferences according to your liking. This adds a personal touch to your chat experience and helps in identifying users and servers easily.

Simply navigate through the chatroom interface to access these functionalities and tailor your chat experience as per your preferences.
