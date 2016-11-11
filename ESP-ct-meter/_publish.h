#include <MqttConnector.h>

extern float t_dht;
extern float h_dht;
extern float data;
extern const char* DEVICE_NAME;
MqttConnector::prepare_data_hook_t on_prepare_data =
[&](JsonObject *root) -> void {

  JsonObject& data = (*root)["d"];
  JsonObject& info = (*root)["info"];


  data["myName"] = DEVICE_NAME;
  data["tags"] = "inside,cmmc";
  data["data"] = data;
  //    data["humid"] = h_dht;
};
