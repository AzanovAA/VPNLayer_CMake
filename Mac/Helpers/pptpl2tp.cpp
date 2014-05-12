#include <stdio.h>
#include <SystemConfiguration/SCSchemaDefinitions.h>
#include <SystemConfiguration/SCNetwork.h>
#include <SystemConfiguration/SCNetworkConnection.h>
#include <SystemConfiguration/SCNetworkConfiguration.h>

CFStringRef initService( CFStringRef type , CFStringRef name, SCPreferencesRef m_prefs );

int main(int argc, char *argv[])
{
    CFStringRef m_appName = CFStringCreateWithCString( NULL
                         , "VPNLayer"
                         , kCFStringEncodingUTF8 );

    CFStringRef m_l2tpServiceName = CFStringCreateWithCString( NULL
                          , "VPNLayer:L2TP"
                          , kCFStringEncodingUTF8 );
    CFStringRef m_pptpServiceName = CFStringCreateWithCString( NULL
                          , "VPNLayer:PPTP"
                          , kCFStringEncodingUTF8 );

    SCPreferencesRef m_prefs = NULL;

    AuthorizationFlags flags = kAuthorizationFlagDefaults
                             | kAuthorizationFlagExtendRights
                             | kAuthorizationFlagInteractionAllowed
                             | kAuthorizationFlagPreAuthorize;
    OSStatus err;
    AuthorizationRef auth;
    err = AuthorizationCreate(0, kAuthorizationEmptyEnvironment, flags, &auth);
    if (err == noErr)
        m_prefs = SCPreferencesCreateWithAuthorization(0, m_appName, 0, auth);
    else
        m_prefs = SCPreferencesCreate(0, m_appName, 0);

    if (!m_prefs) return 0;

    CFStringRef m_l2tpServiceId = NULL;
    CFStringRef m_pptpServiceId = NULL;

    CFArrayRef connArray = SCNetworkServiceCopyAll(m_prefs);
    if (!connArray) return 0;

    for (CFIndex i = 0, sz = CFArrayGetCount(connArray); i < sz; ++i)
    {
        SCNetworkServiceRef service;
        service = (SCNetworkServiceRef) CFArrayGetValueAtIndex(connArray, i);

        CFStringRef serviceName = SCNetworkServiceGetName(service);

        if (kCFCompareEqualTo == CFStringCompare(m_l2tpServiceName
                                   , serviceName
                                   , kCFCompareCaseInsensitive)
        ) { m_l2tpServiceId = SCNetworkServiceGetServiceID(service); }

        if (kCFCompareEqualTo == CFStringCompare(m_pptpServiceName
                                   , serviceName
                                   , kCFCompareCaseInsensitive)
        ) { m_pptpServiceId = SCNetworkServiceGetServiceID(service); }
    }

    CFRelease(connArray);

    if (!m_pptpServiceId)
        m_pptpServiceId = initService( kSCNetworkInterfaceTypePPTP
                                     , m_pptpServiceName, m_prefs );
    if (!m_l2tpServiceId)
        m_l2tpServiceId = initService( kSCNetworkInterfaceTypeL2TP
                                     , m_l2tpServiceName, m_prefs );

	return 0;
}

CFStringRef initService( CFStringRef type , CFStringRef name, SCPreferencesRef m_prefs )
{
    SCPreferencesLock(m_prefs, FALSE);

    SCNetworkInterfaceRef IF;
    IF = SCNetworkInterfaceCreateWithInterface(kSCNetworkInterfaceIPv4, type);
    if (IF)
        IF = SCNetworkInterfaceCreateWithInterface(IF, kSCNetworkInterfaceTypePPP);
    if (!IF) return NULL;

    SCNetworkServiceRef service;
    service = SCNetworkServiceCreate(m_prefs, IF);
    if (!service) return NULL;

    Boolean operationOk;
    operationOk = SCNetworkServiceSetName(service, name);
    if (!operationOk) return NULL;

    operationOk = SCNetworkServiceEstablishDefaultConfiguration(service);
    if (!operationOk) return NULL;

    SCNetworkProtocolRef protocol;
    protocol = SCNetworkServiceCopyProtocol(service, CFSTR("IPv4"));
    if (!protocol) return NULL;

    CFDictionaryRef ip4DialOptions = NULL;
    const void *ip4Keys[1];
    const void *ip4Vals[1];
    const int val = 1;
    ip4Keys[0] = (void *) kSCPropNetOverridePrimary;
    ip4Vals[0] = (void *) CFNumberCreate(NULL, kCFNumberIntType, &val);
    ip4DialOptions = CFDictionaryCreate( NULL
                                       , (const void**)ip4Keys
                                       , (const void**)ip4Vals
                                       , 1
                                       , &kCFTypeDictionaryKeyCallBacks
                                       , &kCFTypeDictionaryValueCallBacks );
    operationOk = SCNetworkProtocolSetConfiguration( protocol, ip4DialOptions );

    SCNetworkSetRef set = SCNetworkSetCopyCurrent(m_prefs);
    if (!set) return NULL;

    if (set && SCNetworkSetAddService(set, service))
    {
        SCPreferencesCommitChanges(m_prefs);
        SCPreferencesApplyChanges(m_prefs);
        SCPreferencesSynchronize(m_prefs);
    }
    CFStringRef serviceId = SCNetworkServiceGetServiceID(service);

    SCPreferencesUnlock(m_prefs);

    CFRelease(set);
    CFRelease(ip4DialOptions);
    CFRelease(protocol);

    return serviceId;
}
