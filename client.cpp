#include "cards.h"
#include <vector>

using namespace std;

int lookup_and_connect(const char *host, const char *service) {
  struct addrinfo hints;
  struct addrinfo *rp, *result;
  int s;

  /* Translate host name into peer's IP address */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  if ((s = getaddrinfo(host, service, &hints, &result)) != 0) {
    fprintf(stderr, "stream-talk-client: getaddrinfo: %s\n", gai_strerror(s));
    return -1;
  }

  /* Iterate through the address list and try to connect */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
      continue;
    }
    if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1) {
      break;
    }
    close(s);
  }

  if (rp == NULL) {
    perror("stream-talk-client: connect");
    return -1;
  }

  freeaddrinfo(result);
  return s;
}

int main(){
  vector<Cards> hand;
  //connect to server
}
