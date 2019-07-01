import sys

sys.path.insert(0, "..")

from opcua import Client

SERVER_ADDRESS = "opc.tcp://172.17.10.15:48010"

if __name__ == "__main__":
    client = Client(SERVER_ADDRESS)
    try:
        client.connect()
        client.load_type_definitions()  # load definition of server specific structures/extension objects

        nodeid = ""
        methodname = ""

        while nodeid != "quit":
            nodeid = input("Enter Node ID or 'quit':")
            if nodeid != "quit":
                try:
                    node = client.get_node(nodeid)
                    print("Found node {}".format(node))
                    while methodname != "back":
                        methodname = input("Enter method name or 'back':")
                        if methodname != "back":
                            try:
                                res = node.call_method("2:{}".format(methodname))
                                print("Method call succeeded")
                                methodname = "back"
                            except:
                                print("Method call failed. Try again.")
                except:
                    print("Invalid node id. Try again.")

    finally:
        client.disconnect()
