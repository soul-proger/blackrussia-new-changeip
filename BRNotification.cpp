#include "BRNotification.h"

std::vector<s_BrNotification> BrNotificationPool;

void BrNotificationUpdate(JNIEnv* env)
{
    if(!BrNotificationPool.empty()) {
        s_BrNotification next_notification = BrNotificationPool.front();
        ShowBrNotification(env, next_notification.type, next_notification.message.c_str(), next_notification.duration);
        BrNotificationPool.erase(BrNotificationPool.begin());
    }
}

void ShowBrNotification(JNIEnv* env, eBrNotificationType type, std::string msg, int duration, std::string text2)
{
    string jsonString = "{ \"t\":" + to_string(type) + ", \"i\":\"" + msg + "\", \"d\":" + to_string(duration) + ", \"s\":1, \"b\":1, \"k\":\"" + text2 + "\" }";
    const char* jsonChars = jsonString.c_str();

    jclass guiManagerClass = env->FindClass("com/blackhub/bronline/game/GUIManager");
    if (guiManagerClass != NULL) {
        jmethodID getInstanceMethod = env->GetStaticMethodID(guiManagerClass, "getInstance", "()Lcom/blackhub/bronline/game/GUIManager;");
        if (getInstanceMethod != NULL) {
            jobject guiManagerInstance = env->CallStaticObjectMethod(guiManagerClass, getInstanceMethod);
            
            jclass jsonObjectClass = env->FindClass("org/json/JSONObject");
            if (jsonObjectClass != NULL) {
                jmethodID jsonConstructor = env->GetMethodID(jsonObjectClass, "<init>", "(Ljava/lang/String;)V");
                if (jsonConstructor != NULL) {
                    jstring jsonStr = env->NewStringUTF(jsonChars);
                    jobject jsonObject = env->NewObject(jsonObjectClass, jsonConstructor, jsonStr);
                    
                    jmethodID handleNotifMethod = env->GetMethodID(guiManagerClass, "handleNotificationScreen", "(ILorg/json/JSONObject;)V");
                    if (handleNotifMethod != NULL) {
                        env->CallVoidMethod(guiManagerInstance, handleNotifMethod, 1, jsonObject);
                    }
                    
                    env->DeleteLocalRef(jsonStr);
                    env->DeleteLocalRef(jsonObject);
                }
            }
            
            env->DeleteLocalRef(guiManagerInstance);
        }
    }
    
    env->DeleteLocalRef(guiManagerClass);
}

void BrNotification(eBrNotificationType type, std::string msg, int duration)
{
    s_BrNotification notif;
    notif.type = type;
    notif.message = msg;
    notif.duration = duration;
    BrNotificationPool.push_back(notif);
}