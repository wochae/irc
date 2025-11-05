#ifndef SOCKETREACTOR_TPP
#define SOCKETREACTOR_TPP

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <cstdlib>

template <typename T>
void SocketReactor<T>::init(T& object, EventHandler onEventSuccess, EventHandler onEventError)
{
	if((_kqueue = kqueue()) < 0)
	{
		std::cerr << "SocketReactor init() : " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	_object = &object;
	_onEventSuccess = onEventSuccess;
	_onEventError = onEventError;
}

template <typename T>
void SocketReactor<T>::addSocket(int clientSocket)
{
	struct kevent event;
	// 읽기 이벤트만 등록
	EV_SET(&event, clientSocket, EVFILT_READ, EV_ADD, 0, 0, 0);
	kevent(_kqueue, &event, 1, 0, 0, 0);
}

template <typename T>
void SocketReactor<T>::removeSocket(int clientSocket)
{
	struct kevent event;
	// READ 필터 제거
	EV_SET(&event, clientSocket, EVFILT_READ, EV_DELETE, 0, 0, 0);
	kevent(_kqueue, &event, 1, 0, 0, 0);
	// 과거에 WRITE를 등록했을 가능성 대비해 제거 시도(있으면 제거, 없으면 영향 없음)
	EV_SET(&event, clientSocket, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
	kevent(_kqueue, &event, 1, 0, 0, 0);
}

template <typename T>
void SocketReactor<T>::run()
{
	const int MAX_EVENT = 100;

	struct kevent events[MAX_EVENT];
	int nEvents = kevent(_kqueue, 0, 0, events, MAX_EVENT, 0);

	for (int i = 0; i < nEvents; ++i)
	{
		_currentEvent = &events[i];

		if (_currentEvent->flags & EV_ERROR)
		{
			(_object->*_onEventError)(_currentEvent->ident);
			continue;
		}
		if (_currentEvent->flags & EV_EOF)
		{
			// 연결 종료 감지 시 소켓 제거 + 정리 핸들러 호출
			removeSocket(_currentEvent->ident);
			(_object->*_onEventError)(_currentEvent->ident);
			continue;
		}
		if (_currentEvent->filter == EVFILT_READ)
		{
			(_object->*_onEventSuccess)(_currentEvent->ident);
		}
	}
}

#endif // SOCKETREACTOR_TPP

template <typename T>
void SocketReactor<T>::init(T& object, EventHandler onEventSuccess, EventHandler onEventError)
{
	if((_kqueue = kqueue()) < 0)
	{
		std::cerr << "SocketReactor init() : " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	_object = &object;
	_onEventSuccess = onEventSuccess;
	_onEventError = onEventError;
}

template <typename T>
void SocketReactor<T>::addSocket(int clientSocket)
{
	struct kevent event;
	EV_SET(&event, clientSocket, EVFILT_READ, EV_ADD, 0, 0, 0);
	kevent(_kqueue, &event, 1, 0, 0, 0);
}

template <typename T>
void SocketReactor<T>::removeSocket(int clientSocket)
{
	struct kevent event;
	EV_SET(&event, clientSocket, EVFILT_READ, EV_DELETE, 0, 0, 0);
	kevent(_kqueue, &event, 1, 0, 0, 0);
	EV_SET(&event, clientSocket, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
	kevent(_kqueue, &event, 1, 0, 0, 0);
}

template <typename T>
void SocketReactor<T>::run()
{
	const int MAX_EVENT = 100;

	struct kevent events[MAX_EVENT];
	int nEvents = kevent(_kqueue, 0, 0, events, MAX_EVENT, 0);

	for (int i = 0; i < nEvents; ++i)
	{
		_currentEvent = &events[i];

		if (_currentEvent->flags & EV_ERROR)
		{
			(_object->*_onEventError)(_currentEvent->ident);
			continue;
		}
		if (_currentEvent->flags & EV_EOF)
		{
			removeSocket(_currentEvent->ident);
			(_object->*_onEventError)(_currentEvent->ident);
			continue;
		}
		if (_currentEvent->filter == EVFILT_READ)
		{
			(_object->*_onEventSuccess)(_currentEvent->ident);
		}
	}
}
