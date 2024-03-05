# MongoDB
## A low level key-value database implementation in C.


request structure:
client_ip: integer
method: char[10]
key: long int
value: char[MAX_VSIZE]

response structure:
client_ip: integer
value: char[MAX_VSIZE]
status_code: integer
info_message: char[200]


