from _animation import *
from entities import Activity

def ADD_ACTIVITY(cls, activity_name):
    setattr( cls, activity_name, Activity(ActivityList_RegisterPrivateActivity(activity_name)) )