#include "HttpRequest.h"
#include "prefs.h"
#include "RequestFactory.h"
#include <cppunit/extensions/HelperMacros.h>

using namespace std;

class HttpRequestTest : public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(HttpRequestTest);
  CPPUNIT_TEST(testGetStartByte);
  CPPUNIT_TEST(testGetEndByte);
  CPPUNIT_TEST(testCreateRequest);
  CPPUNIT_TEST(testCreateRequest_ftp);
  CPPUNIT_TEST(testCreateRequest_with_cookie);
  CPPUNIT_TEST(testCreateProxyRequest);
  CPPUNIT_TEST(testIsRangeSatisfied);
  CPPUNIT_TEST_SUITE_END();
private:

public:
  void setUp() {}

  void testGetStartByte();
  void testGetEndByte();
  void testCreateRequest();
  void testCreateRequest_ftp();
  void testCreateRequest_with_cookie();
  void testCreateProxyRequest();
  void testIsRangeSatisfied();
};


CPPUNIT_TEST_SUITE_REGISTRATION( HttpRequestTest );

void HttpRequestTest::testGetStartByte()
{
  HttpRequest httpRequest;
  SegmentHandle segment = new Segment(1, 1024*1024, 1024*1024, 0);

  CPPUNIT_ASSERT_EQUAL((int64_t)0, httpRequest.getStartByte());

  httpRequest.setSegment(segment);
  
  CPPUNIT_ASSERT_EQUAL((int64_t)1024*1024, httpRequest.getStartByte());

}

void HttpRequestTest::testGetEndByte()
{
  int32_t index = 1;
  int32_t length = 1024*1024-1024;
  int32_t segmentLength = 1024*1024;
  int32_t writtenLength = 1024;

  HttpRequest httpRequest;
  SegmentHandle segment = new Segment(index, length, segmentLength, writtenLength);

  CPPUNIT_ASSERT_EQUAL((int64_t)0, httpRequest.getEndByte());

  httpRequest.setSegment(segment);

  CPPUNIT_ASSERT_EQUAL((int64_t)0,
		       httpRequest.getEndByte());

  RequestHandle request = new Request();
  request->setKeepAlive(true);

  httpRequest.setRequest(request);

  CPPUNIT_ASSERT_EQUAL((int64_t)segmentLength*index+length-1,
		       httpRequest.getEndByte());


  request->setKeepAlive(false);

  CPPUNIT_ASSERT_EQUAL((int64_t)0, httpRequest.getEndByte());
}

void HttpRequestTest::testCreateRequest()
{
  SharedHandle<Option> option = new Option();
  option->put(PREF_HTTP_AUTH_ENABLED, V_FALSE);
  option->put(PREF_HTTP_PROXY_ENABLED, V_FALSE);
  option->put(PREF_HTTP_PROXY_METHOD, V_TUNNEL);
  option->put(PREF_HTTP_PROXY_AUTH_ENABLED, V_FALSE);
  option->put(PREF_HTTP_USER, "aria2user");
  option->put(PREF_HTTP_PASSWD, "aria2passwd");
  option->put(PREF_HTTP_PROXY_USER, "aria2proxyuser");
  option->put(PREF_HTTP_PROXY_PASSWD, "aria2proxypasswd");
  RequestFactory requestFactory;
  requestFactory.setOption(option.get());

  RequestHandle request = requestFactory.createRequest();

  request->setUrl("http://localhost:8080/archives/aria2-1.0.0.tar.bz2");

  SegmentHandle segment = new Segment();

  HttpRequest httpRequest;

  httpRequest.setRequest(request);
  httpRequest.setSegment(segment);

  string expectedText = "GET /archives/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost:8080\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());

  request->setKeepAlive(false);

  expectedText = "GET /archives/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost:8080\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Connection: close\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());

  segment->index = 1;
  segment->length = 1024*1024;
  segment->segmentLength = 1024*1024;
  segment->writtenLength = 0;

  expectedText = "GET /archives/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost:8080\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Connection: close\r\n"
    "Range: bytes=1048576-\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());

  request->setKeepAlive(true);

  expectedText = "GET /archives/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost:8080\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Range: bytes=1048576-2097151\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());

  httpRequest.setSegment(new Segment());

  request->redirectUrl("http://localhost:8080/archives/download/aria2-1.0.0.tar.bz2");

  expectedText = "GET /archives/download/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost:8080\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Referer: http://localhost:8080/archives/aria2-1.0.0.tar.bz2\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());
  
  request->resetUrl();

  // enable http auth
  option->put(PREF_HTTP_AUTH_ENABLED, V_TRUE);

  httpRequest.configure(option.get());

  expectedText = "GET /archives/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost:8080\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Authorization: Basic YXJpYTJ1c2VyOmFyaWEycGFzc3dk\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());

  // enable http proxy auth
  option->put(PREF_HTTP_PROXY_AUTH_ENABLED, V_TRUE);

  httpRequest.configure(option.get());

  expectedText = "GET /archives/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost:8080\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Authorization: Basic YXJpYTJ1c2VyOmFyaWEycGFzc3dk\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());

  option->put(PREF_HTTP_PROXY_ENABLED, V_TRUE);

  httpRequest.configure(option.get());

  expectedText = "GET /archives/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost:8080\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Authorization: Basic YXJpYTJ1c2VyOmFyaWEycGFzc3dk\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());

  option->put(PREF_HTTP_PROXY_METHOD, V_GET);

  httpRequest.configure(option.get());

  expectedText = "GET http://localhost:8080/archives/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost:8080\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Proxy-Connection: close\r\n"
    "Proxy-Authorization: Basic YXJpYTJwcm94eXVzZXI6YXJpYTJwcm94eXBhc3N3ZA==\r\n"
    "Authorization: Basic YXJpYTJ1c2VyOmFyaWEycGFzc3dk\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());

  option->put(PREF_HTTP_PROXY_AUTH_ENABLED, V_FALSE);

  httpRequest.configure(option.get());

  expectedText = "GET http://localhost:8080/archives/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost:8080\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Proxy-Connection: close\r\n"
    "Authorization: Basic YXJpYTJ1c2VyOmFyaWEycGFzc3dk\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());  
}

void HttpRequestTest::testCreateRequest_ftp()
{
  SharedHandle<Option> option = new Option();
  option->put(PREF_HTTP_AUTH_ENABLED, V_FALSE);
  option->put(PREF_HTTP_PROXY_ENABLED, V_FALSE);
  option->put(PREF_HTTP_PROXY_METHOD, V_TUNNEL);
  option->put(PREF_HTTP_PROXY_AUTH_ENABLED, V_FALSE);
  option->put(PREF_HTTP_USER, "aria2user");
  option->put(PREF_HTTP_PASSWD, "aria2passwd");
  option->put(PREF_HTTP_PROXY_USER, "aria2proxyuser");
  option->put(PREF_HTTP_PROXY_PASSWD, "aria2proxypasswd");

  RequestFactory requestFactory;
  requestFactory.setOption(option.get());

  RequestHandle request = requestFactory.createRequest();
  request->setUrl("ftp://localhost:8080/archives/aria2-1.0.0.tar.bz2");

  SegmentHandle segment = new Segment();

  HttpRequest httpRequest;

  httpRequest.setRequest(request);
  httpRequest.setSegment(segment);

  httpRequest.configure(option.get());

  string expectedText = "GET ftp://localhost:8080/archives/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost:8080\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());

  // How to enable HTTP proxy authorization in FTP download via HTTP proxy
  option->put(PREF_HTTP_PROXY_ENABLED, V_TRUE);
  option->put(PREF_HTTP_PROXY_METHOD, V_GET);
  option->put(PREF_HTTP_PROXY_AUTH_ENABLED, V_TRUE);

  httpRequest.configure(option.get());

  expectedText = "GET ftp://localhost:8080/archives/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost:8080\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Proxy-Connection: close\r\n"
    "Proxy-Authorization: Basic YXJpYTJwcm94eXVzZXI6YXJpYTJwcm94eXBhc3N3ZA==\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());
  
}

void HttpRequestTest::testCreateRequest_with_cookie()
{
  RequestHandle request = new Request();
  request->setUrl("http://localhost/archives/aria2-1.0.0.tar.bz2");
  SegmentHandle segment = new Segment();

  Cookie cookie1("name1", "value1", "/archives", "localhost", false);
  Cookie cookie2("name2", "value2", "/archives/download", "localhost", false);
  Cookie cookie3("name3", "value3", "/archives/download", "tt.localhost", false);
  Cookie cookie4("name4", "value4", "/archives/download", "tt.localhost", true);

  request->cookieBox->add(cookie1);
  request->cookieBox->add(cookie2);
  request->cookieBox->add(cookie3);
  request->cookieBox->add(cookie4);

  HttpRequest httpRequest;

  httpRequest.setRequest(request);
  httpRequest.setSegment(segment);

  string expectedText = "GET /archives/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Cookie: name1=value1;\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());

  request->setUrl("http://localhost/archives/download/aria2-1.0.0.tar.bz2");

  expectedText = "GET /archives/download/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: localhost\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Cookie: name1=value1;name2=value2;\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());

  request->setUrl("http://tt.localhost/archives/download/aria2-1.0.0.tar.bz2");

  expectedText = "GET /archives/download/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: tt.localhost\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Cookie: name1=value1;name2=value2;name3=value3;\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());

  request->setUrl("https://tt.localhost/archives/download/aria2-1.0.0.tar.bz2");

  expectedText = "GET /archives/download/aria2-1.0.0.tar.bz2 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Accept: */*\r\n"
    "Host: tt.localhost\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Cookie: name1=value1;name2=value2;name3=value3;name4=value4;\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createRequest());
  
}

void HttpRequestTest::testCreateProxyRequest()
{
  RequestHandle request = new Request();
  request->setUrl("http://localhost/archives/aria2-1.0.0.tar.bz2");
  SegmentHandle segment = new Segment();

  HttpRequest httpRequest;

  httpRequest.setRequest(request);
  httpRequest.setSegment(segment);

  string expectedText = "CONNECT localhost:80 HTTP/1.1\r\n"
    "User-Agent: aria2\r\n"
    "Proxy-Connection: close\r\n"
    "Host: localhost:80\r\n"
    "\r\n";

  CPPUNIT_ASSERT_EQUAL(expectedText, httpRequest.createProxyRequest());
  
}

void HttpRequestTest::testIsRangeSatisfied()
{
  RequestHandle request = new Request();
  request->setUrl("http://localhost:8080/archives/aria2-1.0.0.tar.bz2");
  request->setKeepAlive(false);
  SegmentHandle segment = new Segment();

  HttpRequest httpRequest;

  httpRequest.setRequest(request);
  httpRequest.setSegment(segment);

  RangeHandle range = new Range(0, 0, 0);

  CPPUNIT_ASSERT(httpRequest.isRangeSatisfied(range));

  segment->index = 1;
  segment->length = 1024*1024;
  segment->segmentLength = 1024*1024;
  segment->writtenLength = 0;

  int64_t entityLength = segment->segmentLength*10;

  CPPUNIT_ASSERT(!httpRequest.isRangeSatisfied(range));

  range = new Range(segment->getPosition(), 0, entityLength);

  CPPUNIT_ASSERT(httpRequest.isRangeSatisfied(range));

  httpRequest.setEntityLength(entityLength-1);

  CPPUNIT_ASSERT(!httpRequest.isRangeSatisfied(range));

  httpRequest.setEntityLength(entityLength);

  CPPUNIT_ASSERT(httpRequest.isRangeSatisfied(range));

  request->setKeepAlive(true);

  CPPUNIT_ASSERT(!httpRequest.isRangeSatisfied(range));

  range = new Range(segment->getPosition(),
		    segment->getPosition()+segment->length-1, entityLength);

  CPPUNIT_ASSERT(httpRequest.isRangeSatisfied(range));

  range = new Range(0, segment->getPosition()+segment->length-1, entityLength);

  CPPUNIT_ASSERT(!httpRequest.isRangeSatisfied(range));
}
