/*
* Copyright (c) 2018 Pawel Drzycimski
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

#include <simpleopenssl/simpleopenssl.h>
#include "simplelog/simplelog.h"
#include "cmdline/cmdline.h"
#include <iomanip>


using namespace so;

std::string bin2Hex(const so::Bytes &buff);
std::string bin2Text(const so::Bytes &buff);
void logHex(const std::string &hexStr, size_t newLine);
void handleCert(const std::string &fileName);
void handleCrl(const std::string &fileName);

int main(int argc, char *argv[])
{
  cmdline::parser arg;
  arg.add("help", 'h', "Print help.");
  arg.add<std::string>("file", 'f', "PEM cert or crl file to be printed.", true);
  arg.add<std::string>("type", 't', "Type of input - 'cert' (default) or 'crl'", false);
 
  if(!arg.parse(argc, const_cast<const char* const*>(argv)))
  {
    const auto fullErr = arg.error_full();
    if(!fullErr.empty())
      log << fullErr;
     
    log << arg.usage();
    return 0;
  }
  
  if(arg.exist("help"))
  {
    log << arg.usage();
    return 0;
  } 

  if(!arg.exist("file"))
  {
    log << "--file or -f argument is mandatory!";
    log << arg.usage();
    return 0;
  }
  
  const std::string file = arg.get<std::string>("file");

  const std::string type = [&]{
    if(arg.exist("type"))
      return arg.get<std::string>("type"); 

    return std::string{"cert"};
  }();

  if(type != "cert" && type != "crl")
  {
    log << "--type or -t argument is invalid!";
    log << arg.usage();
    return 0;
  }

  if(type == "cert")
    handleCert(file);
  else if(type == "crl")
    handleCrl(file);

  return 0;
}


void handleCrl(const std::string &fileName)
{
  auto maybeX509 = x509::convertPemFileToCRL(fileName);
  if(!maybeX509)
  {
    log << maybeX509.msg();
    return;
  }

  auto crl = maybeX509.moveValue();

  const auto[version, versionRaw] = x509::getVersion(*crl);
  switch(version)
  {
    case x509::Version::v1:
      log << "Version: 1 (" << versionRaw << ")";
      break;
    case x509::Version::v2:
      log << "Version: 2 (" << versionRaw << ")";
      break;
    case x509::Version::v3:
      log << "Version: 3 (" << versionRaw << ")";
      break;
    case x509::Version::vx:
      log << "Version: " << versionRaw;
      break;
  }
   
  auto maybeIssuer = x509::getIssuer(*crl);
  if(!maybeIssuer)
  {
    log << maybeIssuer.msg();
    return;
  }
  const auto issuer = maybeIssuer.moveValue();
  log << "Issuer:";
  log << "\tCommonName: " << issuer.commonName;
  log << "\tCountryName: " << issuer.countryName;
  log << "\tLocalityName: " << issuer.localityName;
  log << "\tOrganizationName: " << issuer.organizationName;
  log << "\tStateOrProvinceName: " << issuer.stateOrProvinceName;

  const auto extensions = x509::getExtensions(*crl);
  if(!extensions)
  {
    log << extensions.msg();
    return;
  }
  log << "ExtensionCount: " << extensions.value.size();

  if(!extensions.value.empty())
  {
    for(const auto &ext : extensions.value)
    {
      if(ext.id != x509::CrlExtensionId::UNDEF)
      {
        log << "\t" << ext.name << " [" << ext.oidNumerical << "]";
        log << "\t  critical: " << (ext.critical ? "true" : "false");
        log << "\t  " << bin2Text(ext.data);
      }
      else
      {
        log << "\toid: " << ext.oidNumerical;
        log << "\t  critical: " << (ext.critical ? "true" : "false");
        log << "\t  " << bin2Hex(ext.data);
      }
    }
  }

  const auto maybeRevoked = x509::getRevoked(*crl);
  if(!maybeRevoked)
  {
    log << maybeRevoked.msg();
    return;
  }
  
  const auto &revokedList = maybeRevoked.value;
  log << "Revoked Certificates ( " << revokedList.size() << " )" << (revokedList.empty() ? "" : ":");
  if(!revokedList.empty())
  {
    for(const auto &revoked : revokedList)
    {
      log << "\tSerial: " << bin2Hex(revoked.serialNumAsn1); 
      log << "\t  Revocation date: " << revoked.dateISO860;
      if(!revoked.extensions.empty())
        log << "\t  CRL entry extensions:";

      for(const auto &revExt : revoked.extensions)
      {
        if(revExt.id != x509::CrlEntryExtensionId::UNDEF)
        {
          log << "\t\t" << revExt.name << " [" << revExt.oidNumerical << "]";
          log << "\t\t  " << bin2Text(revExt.data);
          if(revExt.critical)
            log << "\t\t  critical: true";
        }
        else
        {
          log << "\t\toid: " << revExt.oidNumerical;
          log << "\t\t  " << bin2Hex(revExt.data);
          if(revExt.critical)
            log << "\t\t  critical: true";
        }        
      }
    }
  }

  const auto sig = x509::getSignature(*crl);
  if(!sig)
  {
    log << sig.msg();
    return;
  } 
  const auto sigType = x509::getSignatureAlgorithm(*crl);
  log << "Signature: " << nid::getLongName(sigType).value;
  logHex(bin2Hex(sig.value), 36);

}

void handleCert(const std::string &fileName)
{
  auto maybeX509 = x509::convertPemFileToX509(fileName);
  if(!maybeX509)
  {
    log << maybeX509.msg();
    return;
  }

  auto cert = maybeX509.moveValue();

  const auto[version, versionRaw] = x509::getVersion(*cert);
  switch(version)
  {
    case x509::Version::v1:
      log << "Version: 1 (" << versionRaw << ")";
      break;
    case x509::Version::v2:
      log << "Version: 2 (" << versionRaw << ")";
      break;
    case x509::Version::v3:
      log << "Version: 3 (" << versionRaw << ")";
      break;
    case x509::Version::vx:
      log << "Version: " << versionRaw;
      break;
  }
  
  const auto serial = x509::getSerialNumber(*cert);
  if(!serial)
  {
    log << serial.msg();
    return;
  }
  log << "Serial: " << bin2Hex(serial.value);

  auto maybeSubject = x509::getSubject(*cert);
  if(!maybeSubject)
  {
    log << maybeSubject.msg();
    return;
  }

  const auto subject = maybeSubject.moveValue();
  log << "Subject:";
  log << "\tCommonName: " << subject.commonName;
  log << "\tCountryName: " << subject.countryName;
  log << "\tLocalityName: " << subject.localityName;
  log << "\tOrganizationName: " << subject.organizationName;
  log << "\tStateOrProvinceName: " << subject.stateOrProvinceName;

  
  auto maybeIssuer = x509::getIssuer(*cert);
  if(!maybeIssuer)
  {
    log << maybeIssuer.msg();
    return;
  }
  const auto issuer = maybeIssuer.moveValue();
  log << "Issuer:";
  log << "\tCommonName: " << issuer.commonName;
  log << "\tCountryName: " << issuer.countryName;
  log << "\tLocalityName: " << issuer.localityName;
  log << "\tOrganizationName: " << issuer.organizationName;
  log << "\tStateOrProvinceName: " << issuer.stateOrProvinceName;

  auto maybePubKey = x509::getPubKey(*cert);
  if(!maybePubKey)
  {
    log << maybePubKey.msg();
    return;
  }
  
  auto pubKey = maybePubKey.moveValue(); 
  const auto pubKeyBytes = evp::convertPubKeyToDer(*pubKey);
  if(!pubKeyBytes)
  {
    log << pubKeyBytes.msg();
    return;
  }
 
  const std::string keyInfo = [&]{
    if(auto rsa = evp::convertToRsa(*pubKey))
    {
      auto key = rsa.moveValue();
      return "(" + std::to_string(static_cast<int>(rsa::getKeyBits(*key).value)) + " bit)";
    }
    else if(auto ec = evp::convertToEcdsa(*pubKey))
    {
      auto key = ec.moveValue();
      return ecdsa::convertCurveToString(ecdsa::getCurve(*key).value).value +
        " (" + std::to_string(ecdsa::getKeySize(*key).value) + " bit)";
    }

    return std::string(); 
  }();

  log << "PublicKey: " << nid::getLongName(x509::getPubKeyAlgorithm(*cert)).value << " " << keyInfo;
  logHex(bin2Hex(pubKeyBytes.value), 30);

  const auto extensions = x509::getExtensions(*cert);
  if(!extensions)
  {
    log << extensions.msg();
    return;
  }
  log << "ExtensionCount: " << extensions.value.size();
  
  if(!extensions.value.empty())
  {
    for(const auto &ext : extensions.value)
    {
      if(ext.id != x509::CertExtensionId::UNDEF)
      {
        log << "\t" << ext.name << " [" << ext.oidNumerical << "]";
        log << "\t  critical: " << (ext.critical ? "true" : "false");
        log << "\t  " << bin2Text(ext.data);
      }
      else
      {
        log << "\toid: " << ext.oidNumerical;
        log << "\t  critical: " << (ext.critical ? "true" : "false");
        log << "\t  " << bin2Hex(ext.data);
      }
    }
  }
 
  const auto sig = x509::getSignature(*cert);
  if(!sig)
  {
    log << sig.msg();
    return;
  } 
  const auto sigType = x509::getSignatureAlgorithm(*cert);
  log << "Signature: " << nid::getLongName(sigType).value;
  logHex(bin2Hex(sig.value), 36);
}

std::string bin2Hex(const so::Bytes &buff)
{
  std::ostringstream oss;
  for(const auto bt : buff){
    oss << std::setfill('0') << std::setw(2) << std::hex << +bt;
  }
  return oss.str(); 
}


std::string bin2Text(const so::Bytes &buff)
{
  std::string ret;
  ret.reserve(buff.size());
  std::transform(buff.begin(), buff.end(), std::back_inserter(ret), [](uint8_t bt) { return static_cast<char>(bt);});
  return ret;
}

void logHex(const std::string &hexStr, size_t newLine)
{
  std::cout << "\t";
  for(size_t i = 1; i <= hexStr.size(); ++i)
    std::cout << hexStr[i-1] << (i%2==0 ? " ":"") << (i%newLine == 0 && i!=hexStr.size() ? "\n\t" : "");

  std::cout << "\n";
}

