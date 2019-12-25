import configparser
import os,stat      

home_dir = os.environ['HOME']
filename = home_dir + "/sfrestcli.cfg"
# filename = "test.cfg"

def save_user_auth_info(username , password):
    config = configparser.ConfigParser() 
    config.add_section('Auth')
    config.set('Auth','username',username) 
    config.set('Auth','password',password) 
    with open(filename , 'w') as f: 
        config.write(f)
    
    os.chmod(filename , stat.S_IRUSR | stat.S_IWUSR)   


def read_user_auth_info():
    try:
        config = configparser.ConfigParser() 
        config.read(filename)
        username = config.get('Auth','username')
        password = config.get('Auth','password')
        return { "username" : username , "password" : password}
    except Exception as e:
        return { "username" : "" , "password" : ""}
    