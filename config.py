# config.py

def can_build(env, platform):
    terrain3d = env.get("terrain3d", "yes")  # Default to 'yes'
    if terrain3d == "no":
        print("Removing Terrain3D from build due to 'terrain3d=no' argument.")
        return False
    return True

def configure(env):
    pass