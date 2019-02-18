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

#include <core/ProcessContext.h>

namespace org {
namespace apache {
namespace nifi {
namespace minifi {
namespace core {

bool ProcessContext::getProperty(const Property &property, std::string &value, const std::shared_ptr<FlowFile> &flow_file) {
  if (!property.supportsExpressionLangauge())
    return getProperty(property.getName(), value);
  auto name = property.getName();
  if (expressions_.find(name) == expressions_.end()) {
    std::string expression_str;
    getProperty(name, expression_str);
    logger_->log_debug("Compiling expression for %s/%s: %s", getProcessorNode()->getName(), name, expression_str);
    expressions_.emplace(name, expression::compile(expression_str));
  }

  minifi::expression::Parameters p(flow_file);
  p.configuration = content_repo_->getConfig();

  value = expressions_[name]( p ).asString();
  return true;
}

bool ProcessContext::getDynamicProperty(const Property &property, std::string &value, const std::shared_ptr<FlowFile> &flow_file) {

  if (!property.supportsExpressionLangauge())
    return getDynamicProperty(property.getName(), value);
  auto name = property.getName();
  if (dynamic_property_expressions_.find(name) == dynamic_property_expressions_.end()) {
    std::string expression_str;
    getDynamicProperty(name, expression_str);
    logger_->log_debug("Compiling expression for %s/%s: %s", getProcessorNode()->getName(), name, expression_str);
    dynamic_property_expressions_.emplace(name, expression::compile(expression_str));
  }

  minifi::expression::Parameters p(flow_file);
  p.configuration = content_repo_->getConfig();

  value = dynamic_property_expressions_[name]( p ).asString();
  return true;
}

} /* namespace core */
} /* namespace minifi */
} /* namespace nifi */
} /* namespace apache */
} /* namespace org */
