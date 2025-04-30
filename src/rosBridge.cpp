/// AMM ROS Bridge
/// (c) 2025 University of Washington, CREST lab

#include <thread>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <sstream>

#include <amm_std.h>
#include <signal.h>
#include "amm/BaseLogger.h"

/// json library
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

/// xml library
#include "tinyxml2.h"

#include "websocket_session.hpp"

extern "C" {
   #include "cl_arguments.c"
}

using namespace AMM;
using namespace std::chrono;
using namespace rapidjson;
using namespace tinyxml2;

// declare DDSManager for this module
const std::string moduleName = "ROS Bridge";
const std::string configFile = "config/ros_bridge_amm.xml";
AMM::DDSManager<void>* mgr = new AMM::DDSManager<void>(configFile);
AMM::UUID m_uuid;

//std::mutex nds_mutex;
std::map<std::string, std::string> nodeDataStorage = {
      {"Cardiovascular_HeartRate", "0"},
      {"Cardiovascular_Arterial_Systolic_Pressure", "0"},
      {"Cardiovascular_Arterial_Diastolic_Pressure", "0"},
      {"BloodChemistry_Oxygen_Saturation", "0"},
      {"Respiration_EndTidalCarbonDioxide", "0"},
      {"Respiratory_Respiration_Rate", "0"},
      {"Energy_Core_Temperature", "0"},
      {"SIM_TIME", "0"},
   };

// initialize module state
int sim_status = 0;  // 0 - initial/reset, 1 - running, 2 - paused
int64_t lastTick = 0;

// websocket session for asynchronous read/write to ROS instance
net::io_context ioc;
auto ws_session = std::make_shared<websocket_session>(ioc); 
bool try_reconnect = true;
bool websocket_connected = false;
bool ros_initialized = false;

const std::string target = "/";

//write data packets to websocket
void writeTestPacket() {
   // MoHSES - ROS - first contact!
   std::string message = "{\"op\":\"publish\",\"topic\":\"/hr/control/speech/say\",\"msg\": {\"text\": \"Greetings from MoHSES\"}}";
   LOG_DEBUG << "Writing message to ROS: " << message;
   ws_session->do_write(message);
}

void writePhysDataPacket() {
   std::string message = "{\"op\":\"publish\",\"topic\":\"/hr/control/speech/say\",\"msg\": {\"text\": \"My heart rate is " + nodeDataStorage["Cardiovascular_HeartRate"] + " bpm.\"}}";
   LOG_DEBUG << "Writing message to ROS: " << message;
   ws_session->do_write(message);
}

// callback function for new data on websocket
void onNewWebsocketMessage(const std::string body) {
   // parse web socket message as json data
   std::string type;
   Document document;
   document.Parse(body.c_str());

   if (document.HasMember("type") && document["type"].IsString()) {
      type = document["type"].GetString();
      if (type.compare("ros_topic") == 0) {
         // ignore. only log message type
         LOG_DEBUG << "ros message: {\"type\": \"ros_topic\", ...}";
         return;
      }
      LOG_DEBUG << "ROS message: " << body ;
   } else {
      LOG_ERROR << "ROS message (no type): " << body ;
   }
}

// init ROS comms 
void onWebsocketHandshake(const std::string body) {
   websocket_connected = true;
   writeTestPacket();
}

void OnNewSimulationControl(AMM::SimulationControl& simControl, eprosima::fastrtps::SampleInfo_t* info) {
   std::string message;

   switch (simControl.type()) {
      case AMM::ControlType::RUN :
         //writeRunSimPacket();
         sim_status = 1;
         LOG_INFO << "SimControl Message recieved; Run sim.";
         break;

      case AMM::ControlType::HALT :
         sim_status = 2;
         LOG_INFO << "SimControl Message recieved; Halt sim.";
         break;

      case AMM::ControlType::RESET :
         //TODO: clear data and send to ROS before stopping
         nodeDataStorage.clear();
         sim_status = 0;
         //writeResetSimPacket();
         LOG_INFO << "SimControl Message recieved; Reset sim.";
         break;

      case AMM::ControlType::SAVE :
         // no action
         //LOG_INFO << "SimControl Message recieved; Save sim.";
         //mgr->WriteModuleConfiguration(currentState.mc);
         break;
   }
}

void OnNewTick(AMM::Tick& tick, eprosima::fastrtps::SampleInfo_t* info) {
   //if ( arguments.verbose )
   //   LOG_DEBUG << "Tick received!";
   if ( sim_status == 0 && tick.frame() > lastTick) {
      LOG_DEBUG << "Tick received! sim_status:" << sim_status << "->1 lastTick:" << lastTick << " tick.frame(): " << tick.frame();
      sim_status = 1;
      if ( websocket_connected && ros_initialized ); //writeStateChangePacket(sim_status);
   }
   lastTick = tick.frame();
}

void OnPhysiologyValue(AMM::PhysiologyValue& physiologyvalue, eprosima::fastrtps::SampleInfo_t* info){
   //const std::lock_guard<std::mutex> lock(nds_mutex);
   // store all received phys values
   if (!std::isnan(physiologyvalue.value())) {
      nodeDataStorage[physiologyvalue.name()] = std::to_string(physiologyvalue.value());
      //if ( arguments.verbose )
      //   LOG_DEBUG << "[AMM_Node_Data] " << physiologyvalue.name() << " = " << physiologyvalue.value();
      // phys values are updated every 200ms (5Hz)
      // forward to ROS only once per data update
      // to reduce frequency
      if (physiologyvalue.name()=="SIM_TIME") {
         // reformat SIM_TIME for storage
         std::ostringstream oss;
         oss.precision(1);
         oss << std::fixed << physiologyvalue.value();
         nodeDataStorage["SIM_TIME"] = oss.str();
         //LOG_DEBUG << "sim time stringstream: " << oss.str();
         // send data if websocket connection to ros is live
         if ( websocket_connected ) writePhysDataPacket();
      }
   }

   static bool printRRdata = true;  // set flag to print only initial value received
   if (physiologyvalue.name()=="Respiratory_Respiration_Rate"){
      if ( arguments.verbose && printRRdata ) {
         LOG_DEBUG << "[AMM_Node_Data] Respiratory_Respiration_Rate" << "=" << physiologyvalue.value();
         printRRdata = false;
      }
   }
}

void OnPhysiologyWaveform(AMM::PhysiologyWaveform &waveform, SampleInfo_t *info) {
   // testing mohses data connection
   static int printHFdata = 10;   // initialize counter to print first xx high freequency data points
   if ( arguments.verbose && printHFdata > 0) {
      LOG_DEBUG << "[AMM_Node_Data](HF) " << waveform.name() << "=" << waveform.value();
      printHFdata -= 1;
   }
}

void OnNewRenderModification(AMM::RenderModification &rendMod, SampleInfo_t *info) {
   // LOG_DEBUG << "Render Modification received:\n"
   //          << "Type:      " << rendMod.type() << "\n"
   //          << "Data:      " << rendMod.data();
   // if ( rendMod.type()=="PATIENT_STATE_TACHYPNEA" ) {
   //    etco2Waveform = 2;
   //    LOG_INFO << "Patient entered state: Tachypnea. Setting EtCO2 waveform to 2 -> Obstructive 2";
   //    // waveform type updated on monitor with next ChangeActionPacket
   // }
   if ( rendMod.type()=="PATIENT_STATE_TACHYCARDIA" ) {
      //ecgWaveform = 14;
      //LOG_INFO << "Patient entered state: Tachycardia. Setting ECG waveform to 14 -> Ventricular Tachycardia";
   }
}

// Sample physmod payload for physmod type "airwayobstruction"
// <?xml version="1.0" encoding="UTF-8"?><PhysiologyModification type="AirwayObstruction"><Severity>0.5</Severity></PhysiologyModification>
void OnNewPhysiologyModification(AMM::PhysiologyModification &physMod, SampleInfo_t *info) {
   // LOG_DEBUG << "Physiology Modification received:\n"
   //          << "Type:      " << physMod.type() << "\n"
   //          << "Data:      " << physMod.data();
   tinyxml2::XMLDocument doc;
   doc.Parse(physMod.data().c_str());

   if (doc.ErrorID() == 0) {
      tinyxml2::XMLElement* pRoot;

      pRoot = doc.FirstChildElement("PhysiologyModification");

      while (pRoot) {
         std::string pmType = pRoot->ToElement()->Attribute("type");
         boost::algorithm::to_lower(pmType);
         //LOG_INFO << "Physmod type " << pmType;

         if (pmType == "airwayobstruction") {
            // Type:      PhysiologyModification
            // Data:      <?xml version="1.0" encoding="UTF-8"?><PhysiologyModification type="AirwayObstruction"><Severity>0.5</Severity></PhysiologyModification>
            double pmSev = std::stod(pRoot->FirstChildElement("Severity")->ToElement()->GetText());
            LOG_INFO << "Physiology Modification received: AirwayObstruction. Severity:" << pmSev;
            // forward physmod to ROS or translate to some other action
            return;
         } else {
            LOG_DEBUG << "Physiology Modification received:\n"
                     << "Type:      " << pmType << "\n"
                     << "Data:      " << physMod.data();
            return;
         }
      }
   } else {
      //LOG_ERROR << "Document parsing error, ID: " << doc.ErrorID();
      //doc.PrintError();
   }
}

void PublishOperationalDescription() {
    AMM::OperationalDescription od;
    od.name(moduleName);
    od.model("ROS Bridge");
    od.manufacturer("CREST");
    od.serial_number("0000");
    od.module_id(m_uuid);
    od.module_version("0.1.0");
    od.description("A bridge module to connect MoHSES to a ROS instance.");
    const std::string capabilities = AMM::Utility::read_file_to_string("config/ros_bridge_capabilities.xml");
    od.capabilities_schema(capabilities);
    od.description();
    mgr->WriteOperationalDescription(od);
}

void PublishConfiguration() {
    AMM::ModuleConfiguration mc;
    auto ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    mc.timestamp(ms);
    mc.module_id(m_uuid);
    mc.name(moduleName);
    const std::string configuration = AMM::Utility::read_file_to_string("config/ros_bridge_configuration.xml");
    mc.capabilities_configuration(configuration);
    mgr->WriteModuleConfiguration(mc);
}

void checkForExit() {
   // wait for key press
   std::cin.get();
   std::cout << "Key pressed ... Shutting down." << std::endl;

   // stop() will cause run() to return and leave the reconnect loop
   try_reconnect = false;
   ioc.stop();

   // Raise SIGTERM to trigger async signal handler
   //std::raise(SIGTERM);
}

int main(int argc, char *argv[]) {

   // set default command line options. process.
   arguments.hostname = (char *)"10.0.0.195";
   arguments.port = (char *)"9090";
   arguments.autostart = false;
   arguments.verbose = false;
   argp_parse(&argp, argc, argv, 0, 0, &arguments);

   static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
   plog::init(plog::verbose, &consoleAppender);

   LOG_INFO << "=== [ ROS Bridge ] ===";
   LOG_INFO << "Host IP number = " << arguments.hostname;
   LOG_INFO << "Host port = " << arguments.port;

   mgr->InitializeOperationalDescription();
   mgr->CreateOperationalDescriptionPublisher();

   mgr->InitializeModuleConfiguration();
   mgr->CreateModuleConfigurationPublisher();

   mgr->InitializeSimulationControl();
   mgr->CreateSimulationControlSubscriber(&OnNewSimulationControl);

   mgr->InitializeStatus();
   mgr->CreateStatusPublisher();

   mgr->InitializeTick();
   mgr->CreateTickSubscriber(&OnNewTick);

   mgr->InitializePhysiologyValue();
   mgr->CreatePhysiologyValueSubscriber(&OnPhysiologyValue);

   mgr->InitializePhysiologyWaveform();
   mgr->CreatePhysiologyWaveformSubscriber(&OnPhysiologyWaveform);

   mgr->InitializeRenderModification();
   mgr->CreateRenderModificationSubscriber(&OnNewRenderModification);

   mgr->InitializePhysiologyModification();
   mgr->CreatePhysiologyModificationSubscriber(&OnNewPhysiologyModification);

   m_uuid.id(mgr->GenerateUuidString());

   std::this_thread::sleep_for(milliseconds(250));

   PublishOperationalDescription();
   PublishConfiguration();

   // set up thread to check console for "exit" command
   std::thread ec(checkForExit);
   ec.detach();

   //TODO: turn discovery into asynch process see mycroft bridge
   // block until ROS instance has been discovered on the local network

   LOG_INFO << "ROS Bridge ready.";
   std::cout << "Listening for data... Press return to exit." << std::endl;

   while (try_reconnect) {
      // set up websocket session
      ws_session->run(arguments.hostname, arguments.port, target);
      ws_session->set_verbose( arguments.verbose );
      ws_session->registerHandshakeCallback(onWebsocketHandshake);
      ws_session->registerReadCallback(onNewWebsocketMessage);

      LOG_INFO << "Connecting to ROS instance.";
      // Run the I/O context.
      // The call will return if connection fails or when the socket is closed.
      ioc.run();

      LOG_INFO << "Connection to ROS instance closed.";
      websocket_connected = false;
      ros_initialized = false;
      ioc.reset();
      // wait a while before trying to reconnect
      // TODO skip delay on shutdown
      std::this_thread::sleep_for(seconds(5));
   }

   mgr->Shutdown();
   std::this_thread::sleep_for(milliseconds(100));
   delete mgr;

   LOG_INFO << "ROS Bridge shutdown.";
   return EXIT_SUCCESS;
}
