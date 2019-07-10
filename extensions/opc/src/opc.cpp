/**
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//OPC includes
#include "open62541/open62541.h"
#include "opc.h"

//MiNiFi includes
#include "utils/StringUtils.h"
#include "logging/Logger.h"
#include "Exception.h"

//Standard includes
#include <stdlib.h>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

namespace org {
namespace apache {
namespace nifi {
namespace minifi {
namespace opc {

/*
 * The following functions are only used internally in OPC lib, not to be exported
 */

void add_value_to_variant(UA_Variant *variant, std::string& value) {
  UA_String ua_value = UA_STRING(&value[0]);
  UA_Variant_setScalarCopy(variant, &ua_value, &UA_TYPES[UA_TYPES_STRING]);
}

void add_value_to_variant(UA_Variant *variant, const char* value) {
  std::string strvalue(value);
  add_value_to_variant(variant, strvalue);
}

void add_value_to_variant(UA_Variant *variant, int64_t value) {
  UA_Int64 ua_value = value;
  UA_Variant_setScalarCopy(variant, &ua_value, &UA_TYPES[UA_TYPES_INT64]);
}

void add_value_to_variant(UA_Variant *variant, uint64_t value) {
  UA_UInt64 ua_value = value;
  UA_Variant_setScalarCopy(variant, &ua_value, &UA_TYPES[UA_TYPES_UINT64]);
}

void add_value_to_variant(UA_Variant *variant, int32_t value) {
  UA_Int32 ua_value = value;
  UA_Variant_setScalarCopy(variant, &ua_value, &UA_TYPES[UA_TYPES_INT32]);
}

void add_value_to_variant(UA_Variant *variant, uint32_t value) {
  UA_UInt32 ua_value = value;
  UA_Variant_setScalarCopy(variant, &ua_value, &UA_TYPES[UA_TYPES_UINT32]);
}

void add_value_to_variant(UA_Variant *variant, bool value) {
  UA_Boolean ua_value = value;
  UA_Variant_setScalarCopy(variant, &ua_value, &UA_TYPES[UA_TYPES_BOOLEAN]);
}

void add_value_to_variant(UA_Variant *variant, float value) {
  UA_Float ua_value = value;
  UA_Variant_setScalarCopy(variant, &ua_value, &UA_TYPES[UA_TYPES_FLOAT]);
}

void add_value_to_variant(UA_Variant *variant, double value) {
  UA_Double ua_value = value;
  UA_Variant_setScalarCopy(variant, &ua_value, &UA_TYPES[UA_TYPES_DOUBLE]);
}

/*
 * End of internal functions
 */


template <typename T>
UA_StatusCode update_node(ClientPtr& clientPtr, const UA_NodeId nodeId, T value) {
  UA_Variant *variant = UA_Variant_new();
  add_value_to_variant(variant, value);
  UA_StatusCode sc = UA_Client_writeValueAttribute(clientPtr.get(), nodeId, variant);
  UA_Variant_delete(variant);
  return sc;
};

template UA_StatusCode update_node<int64_t>(ClientPtr& clientPtr, const UA_NodeId nodeId, int64_t value);
template UA_StatusCode update_node<uint64_t>(ClientPtr& clientPtr, const UA_NodeId nodeId, uint64_t value);
template UA_StatusCode update_node<int32_t>(ClientPtr& clientPtr, const UA_NodeId nodeId, int32_t value);
template UA_StatusCode update_node<uint32_t>(ClientPtr& clientPtr, const UA_NodeId nodeId, uint32_t value);
template UA_StatusCode update_node<float>(ClientPtr& clientPtr, const UA_NodeId nodeId, float value);
template UA_StatusCode update_node<double>(ClientPtr& clientPtr, const UA_NodeId nodeId, double value);
template UA_StatusCode update_node<bool>(ClientPtr& clientPtr, const UA_NodeId nodeId, bool value);
template UA_StatusCode update_node<const char *>(ClientPtr& clientPtr, const UA_NodeId nodeId, const char * value);
template UA_StatusCode update_node<std::string>(ClientPtr& clientPtr, const UA_NodeId nodeId, std::string value);

template <typename T>
UA_StatusCode add_node(ClientPtr& clientPtr, const UA_NodeId parentNodeId, const UA_NodeId targetNodeId, std::string browseName, T value, OPCNodeDataType dt, UA_NodeId *receivedNodeId)
{
  //UA_NODEID_NUMERIC(1, 0)
  UA_VariableAttributes attr = UA_VariableAttributes_default;
  add_value_to_variant(&attr.value, value);
  char local[6] = "en-US";
  attr.displayName = UA_LOCALIZEDTEXT(local, const_cast<char*>(browseName.c_str()));
  return UA_Client_addVariableNode(clientPtr.get(),
      targetNodeId,
      parentNodeId,
      UA_NODEID_NUMERIC(0, OPCNodeDataTypeToTypeID(dt)),
      UA_QUALIFIEDNAME(1, const_cast<char*>(browseName.c_str())),
      UA_NODEID_NULL,
      attr, receivedNodeId);
}

template UA_StatusCode add_node<int64_t>(ClientPtr& clientPtr, const UA_NodeId parentNodeId, const UA_NodeId targetNodeId, std::string browseName, int64_t value, OPCNodeDataType dt, UA_NodeId *receivedNodeId);
template UA_StatusCode add_node<uint64_t>(ClientPtr& clientPtr, const UA_NodeId parentNodeId, const UA_NodeId targetNodeId, std::string browseName, uint64_t value, OPCNodeDataType dt, UA_NodeId *receivedNodeId);
template UA_StatusCode add_node<int32_t>(ClientPtr& clientPtr, const UA_NodeId parentNodeId, const UA_NodeId targetNodeId, std::string browseName, int32_t value, OPCNodeDataType dt, UA_NodeId *receivedNodeId);
template UA_StatusCode add_node<uint32_t>(ClientPtr& clientPtr, const UA_NodeId parentNodeId, const UA_NodeId targetNodeId, std::string browseName, uint32_t value, OPCNodeDataType dt, UA_NodeId *receivedNodeId);
template UA_StatusCode add_node<float>(ClientPtr& clientPtr, const UA_NodeId parentNodeId, const UA_NodeId targetNodeId, std::string browseName, float value, OPCNodeDataType dt, UA_NodeId *receivedNodeId);
template UA_StatusCode add_node<double>(ClientPtr& clientPtr, const UA_NodeId parentNodeId, const UA_NodeId targetNodeId, std::string browseName, double value, OPCNodeDataType dt, UA_NodeId *receivedNodeId);
template UA_StatusCode add_node<bool>(ClientPtr& clientPtr, const UA_NodeId parentNodeId, const UA_NodeId targetNodeId, std::string browseName, bool value, OPCNodeDataType dt, UA_NodeId *receivedNodeId);
template UA_StatusCode add_node<const char *>(ClientPtr& clientPtr, const UA_NodeId parentNodeId, const UA_NodeId targetNodeId, std::string browseName, const char * value, OPCNodeDataType dt, UA_NodeId *receivedNodeId);
template UA_StatusCode add_node<std::string>(ClientPtr& clientPtr, const UA_NodeId parentNodeId, const UA_NodeId targetNodeId, std::string browseName, std::string value, OPCNodeDataType dt, UA_NodeId *receivedNodeId);

int32_t OPCNodeDataTypeToTypeID(OPCNodeDataType dt) {
  switch(dt)
  {
    case OPCNodeDataType::Boolean:
      return UA_NS0ID_BOOLEAN;
    case OPCNodeDataType::Int32:
      return UA_NS0ID_INT32;
    case OPCNodeDataType::UInt32:
      return UA_NS0ID_UINT32;
    case OPCNodeDataType::Int64:
      return UA_NS0ID_INT64;
    case OPCNodeDataType::UInt64:
      return UA_NS0ID_UINT64;
    case OPCNodeDataType::Float:
      return UA_NS0ID_FLOAT;
    case OPCNodeDataType::Double:
      return UA_NS0ID_DOUBLE;
    case OPCNodeDataType::String:
      return UA_NS0ID_STRING;
    default:
      throw Exception(OPC_EXCEPTION, "Data type is not supported");
  }
}

void disconnect(UA_Client *client) {
  if(client == nullptr) {
    return;
  }
  if(UA_Client_getState(client) != UA_CLIENTSTATE_DISCONNECTED) {
    UA_Client_disconnect(client);
  }
  UA_Client_delete(client);
}

void setCertificates(ClientPtr& clientPtr, const std::vector<char>& certBuffer, const std::vector<char>& keyBuffer) {
  UA_ClientConfig *cc = UA_Client_getConfig(clientPtr.get());
  cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;

  UA_ByteString certByteString = UA_BYTESTRING_ALLOC(certBuffer.data());
  UA_ByteString keyByteString = UA_BYTESTRING_ALLOC(keyBuffer.data());

  /*UA_ClientConfig_setDefaultEncryption(cc, certByteString, keyByteString,
                                       nullptr, 0,
                                       nullptr, 0);*/

  UA_ByteString_delete(&certByteString);
  UA_ByteString_delete(&keyByteString);
}

ClientPtr connect(const std::string& url, const std::shared_ptr<core::logging::Logger>& logger, const std::string& username, const std::string& password) {
  UA_Client *client = UA_Client_new();
  UA_ClientConfig_setDefault(UA_Client_getConfig(client));
  UA_StatusCode retval;
  if(username.empty()) {
    retval = UA_Client_connect(client, url.c_str());
  } else {
    retval = UA_Client_connect_username(client, "opc.tcp://localhost:53530/OPCUA/SimulationServer",
        username.c_str(), password.c_str());
  }
  if (retval != UA_STATUSCODE_GOOD) {
    logger->log_error("Failed to connect to %s (%s)", url.c_str(), UA_StatusCode_name(retval));
    UA_Client_delete(client);
    return ClientPtr(nullptr, &disconnect);
  }
  logger->log_info("Successfully connected to %s", url.c_str());
  return ClientPtr(client, &disconnect);
}

bool isConnected(const ClientPtr &ptr) {
  UA_Client* client = ptr.get();
  if(!client ) {
    return false;
  }
  return UA_Client_getState(client) != UA_CLIENTSTATE_DISCONNECTED;
}

void traverse(ClientPtr& clientPtr, UA_NodeId nodeId, std::function<nodeFoundCallBackFunc> cb, const std::string& basePath) {

  UA_Client *client = clientPtr.get();
  UA_BrowseRequest bReq;
  UA_BrowseRequest_init(&bReq);
  bReq.requestedMaxReferencesPerNode = 0;
  bReq.nodesToBrowse = UA_BrowseDescription_new();
  bReq.nodesToBrowseSize = 1;

  UA_NodeId_copy(&nodeId, &bReq.nodesToBrowse[0].nodeId);
  bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;

  UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);

  UA_BrowseRequest_deleteMembers(&bReq);

  for(size_t i = 0; i < bResp.resultsSize; ++i) {
    for(size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
      UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
      if (cb(clientPtr, ref, basePath)) {
        if (ref->nodeClass == UA_NODECLASS_VARIABLE || ref->nodeClass == UA_NODECLASS_OBJECT) {
          std::string browsename((char *) ref->browseName.name.data, ref->browseName.name.length);
          traverse(clientPtr, ref->nodeId.nodeId, cb, basePath + browsename);
        }
      } else {
        UA_BrowseResponse_deleteMembers(&bResp);
        return;
      }
    }
  }

  UA_BrowseResponse_deleteMembers(&bResp);
};

bool exists(ClientPtr& clientPtr, UA_NodeId nodeId) {
  bool retval = false;
  auto callback = [&retval](ClientPtr& clientPtr, const UA_ReferenceDescription *ref, const std::string& pat) -> bool {
    retval = true;
    return false;  // If any node is found, the given node exists, so traverse can be stopped
  };
  traverse(clientPtr, nodeId, callback);
  return retval;
};

UA_StatusCode translateBrowsePathsToNodeIdsRequest(ClientPtr& clientPtr, const std::string& path, std::vector<UA_NodeId>& foundNodeIDs, const std::shared_ptr<core::logging::Logger>& logger) {
  logger->log_trace("Trying to find node id for %s", path.c_str());

  UA_Client *client = clientPtr.get();

  auto tokens = utils::StringUtils::split(path, "/");
  std::vector<UA_UInt32> ids;
  for(size_t i = 0; i < tokens.size(); ++i) {
    UA_UInt32 val = (i ==0 ) ? UA_NS0ID_ORGANIZES : UA_NS0ID_HASCOMPONENT;
    ids.push_back(val);
  }

  UA_BrowsePath browsePath;
  UA_BrowsePath_init(&browsePath);
  browsePath.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

  browsePath.relativePath.elements = (UA_RelativePathElement*)UA_Array_new(tokens.size(), &UA_TYPES[UA_TYPES_RELATIVEPATHELEMENT]);
  browsePath.relativePath.elementsSize = tokens.size();

  for(size_t i = 0; i < tokens.size(); ++i) {
    UA_RelativePathElement *elem = &browsePath.relativePath.elements[i];
    elem->referenceTypeId = UA_NODEID_NUMERIC(0, ids[i]);
    elem->targetName = UA_QUALIFIEDNAME_ALLOC(0, tokens[i].c_str());
  }

  UA_TranslateBrowsePathsToNodeIdsRequest request;
  UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);
  request.browsePaths = &browsePath;
  request.browsePathsSize = 1;

  UA_TranslateBrowsePathsToNodeIdsResponse response = UA_Client_Service_translateBrowsePathsToNodeIds(client, request);

  if(response.resultsSize < 1) {
    logger->log_warn("No node id in response for %s", path.c_str());
    return UA_STATUSCODE_BADNODATAAVAILABLE;
  }

  bool foundData = false;

  for(size_t i = 0; i < response.resultsSize; ++i) {
    UA_BrowsePathResult res = response.results[i];
    for(size_t j = 0; j < res.targetsSize; ++j) {
      foundData = true;
      UA_NodeId resultId;
      UA_NodeId_copy(&res.targets[j].targetId.nodeId, &resultId);
      foundNodeIDs.push_back(resultId);
      std::string namespaceUri((char*)res.targets[j].targetId.namespaceUri.data, res.targets[j].targetId.namespaceUri.length);
    }
  }

  UA_BrowsePath_deleteMembers(&browsePath);
  UA_TranslateBrowsePathsToNodeIdsResponse_deleteMembers(&response);

  if(foundData) {
    logger->log_debug("Found %lu nodes for path %s", foundNodeIDs.size(), path.c_str());
    return UA_STATUSCODE_GOOD;
  } else {
    logger->log_warn("No node id found for path %s", path.c_str());
    return UA_STATUSCODE_BADNODATAAVAILABLE;
  }
}

nodeData getNodeData(opc::ClientPtr& clientPtr, const UA_ReferenceDescription *ref, const std::string& basePath) {
  if(ref->nodeClass == UA_NODECLASS_VARIABLE)
  {
    opc::nodeData nodedata;
    std::string browsename(reinterpret_cast<const char*>(ref->browseName.name.data), ref->browseName.name.length);

    if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
      std::string nodeidstr(reinterpret_cast<const char*>(ref->nodeId.nodeId.identifier.string.data),
                            ref->nodeId.nodeId.identifier.string.length);
      nodedata.attributes["NodeID"] = nodeidstr;
      nodedata.attributes["NodeID type"] = "string";
    } else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
        std::string nodeidstr(reinterpret_cast<const char*>(ref->nodeId.nodeId.identifier.string.data), ref->nodeId.nodeId.identifier.string.length);
        nodedata.attributes["NodeID"] = nodeidstr;
        nodedata.attributes["NodeID type"] = "bytestring";
    } else if (ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
        nodedata.attributes["NodeID"] = std::to_string(ref->nodeId.nodeId.identifier.numeric);
        nodedata.attributes["NodeID type"] = "numeric";
    }
    nodedata.attributes["Browsename"] = browsename;
    nodedata.attributes["Full path"] = basePath + "/" + browsename;
    nodedata.dataTypeID = UA_TYPES_COUNT;
    UA_Variant* var = UA_Variant_new();
    if(UA_Client_readValueAttribute(clientPtr.get(), ref->nodeId.nodeId, var) == UA_STATUSCODE_GOOD && var->type != NULL && var->data != NULL) {
      nodedata.dataTypeID = var->type->typeIndex;
      if(var->type->typeName) {
        nodedata.attributes["Typename"] = std::string(var->type->typeName);
      }
      if(var->type->memSize) {
        nodedata.attributes["Datasize"] = std::to_string(var->type->memSize);
        nodedata.data = std::vector<uint8_t>(var->type->memSize);
        memcpy(&nodedata.data[0], var->data, var->type->memSize);
      }
    }
    nodedata.addVariant(var);
    //UA_Variant_delete(var);         f
    return nodedata;
  } else {
    throw Exception(OPC_EXCEPTION, "Only variable nodes are supported!");
  }
}

std::string nodeValue2String(const nodeData& nd) {
  std::string ret_val;
  switch (nd.dataTypeID) {
    case UA_TYPES_STRING:
    case UA_TYPES_LOCALIZEDTEXT:
    case UA_TYPES_BYTESTRING: {
      UA_String value = *(UA_String * )(nd.var_->data);
      ret_val = std::string(reinterpret_cast<const char *>(value.data), value.length);
      break;
    }
    case UA_TYPES_BOOLEAN:
      bool b;
      memcpy(&b, nd.data.data(), sizeof(bool));
      ret_val = b ? "True" : "False";
      break;
    case UA_TYPES_SBYTE:
      int8_t i8t;
      memcpy(&i8t, nd.data.data(), sizeof(i8t));
      ret_val = std::to_string(i8t);
      break;
    case UA_TYPES_BYTE:
      uint8_t ui8t;
      memcpy(&ui8t, nd.data.data(), sizeof(ui8t));
      ret_val = std::to_string(ui8t);
      break;
    case UA_TYPES_INT16:
      int16_t i16t;
      memcpy(&i16t, nd.data.data(), sizeof(i16t));
      ret_val = std::to_string(i16t);
      break;
    case UA_TYPES_UINT16:
      uint16_t ui16t;
      memcpy(&ui16t, nd.data.data(), sizeof(ui16t));
      ret_val = std::to_string(ui16t);
      break;
    case UA_TYPES_INT32:
      int32_t i32t;
      memcpy(&i32t, nd.data.data(), sizeof(i32t));
      ret_val = std::to_string(i32t);
      break;
    case UA_TYPES_UINT32:
      uint32_t ui32t;
      memcpy(&ui32t, nd.data.data(), sizeof(ui32t));
      ret_val = std::to_string(ui32t);
      break;
    case UA_TYPES_INT64:
      int64_t i64t;
      memcpy(&i64t, nd.data.data(), sizeof(i64t));
      ret_val = std::to_string(i64t);
      break;
    case UA_TYPES_UINT64:
      uint64_t ui64t;
      memcpy(&ui64t, nd.data.data(), sizeof(ui64t));
      ret_val = std::to_string(ui64t);
      break;
    case UA_TYPES_FLOAT:
      if(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559){
        float f;
        memcpy(&f, nd.data.data(), sizeof(float));
        ret_val = std::to_string(f);
      } else {
        throw Exception(OPC_EXCEPTION, "Float is non-standard on this system, OPC data cannot be extracted!");
      }
      break;
    case UA_TYPES_DOUBLE:
      if(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559){
        double d;
        memcpy(&d, nd.data.data(), sizeof(double));
        ret_val = std::to_string(d);
      } else {
        throw Exception(OPC_EXCEPTION, "Double is non-standard on this system, OPC data cannot be extracted!");
      }
      break;
    case UA_TYPES_DATETIME: {
      UA_DateTime dt;
      memcpy(&dt, nd.data.data(), sizeof(UA_DateTime));
      ret_val = opc::OPCDateTime2String(dt);
      break;
    }
    default:
      throw Exception(OPC_EXCEPTION, "Data type is not supported ");
  }
  return ret_val;
}

std::string OPCDateTime2String(UA_DateTime raw_date) {
  UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
  std::array<char, 100> charBuf;

  snprintf(&charBuf[0], charBuf.size(), "%u-%u-%u %u:%u:%u.%03u", dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);

  return std::string(charBuf.data(), charBuf.size());
}

} /* namespace opc */
} /* namespace minifi */
} /* namespace nifi */
} /* namespace apache */
} /* namespace org */