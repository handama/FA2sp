import secrets
import string

def generate_cmd_safe_password(length=32):
    safe_chars = (
        'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
        'abcdefghijklmnopqrstuvwxyz'
        '0123456789'
        '_+@#~!?-'
    )
    password = ''.join(secrets.choice(safe_chars) for _ in range(length))
    return password

if __name__ == "__main__":
    passwd = generate_cmd_safe_password()
    print("32 Byte AES Key：")
    print(passwd)
