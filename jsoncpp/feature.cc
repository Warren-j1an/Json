#include "feature.h"

namespace JsonCPP {
Features::Features() : m_allowComments(true), m_strictRoot(false),
    m_allowDroppedNullPlaceholders(false), m_allowNumericKeys(false) {}

Features Features::All() {
    return {};
}

Features Features::StrictMode() {
    Features features;
    features.m_allowComments = false;
    features.m_strictRoot = true;
    features.m_allowDroppedNullPlaceholders = false;
    features.m_allowNumericKeys = false;
    return features;
}
}