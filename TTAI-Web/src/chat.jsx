import { StrictMode, useCallback, useEffect, useLayoutEffect, useMemo, useRef, useState } from 'react'
import { createRoot } from 'react-dom/client'
import { fetchEventSource } from '@microsoft/fetch-event-source'
import ReactMarkdown from 'react-markdown'
import remarkBreaks from 'remark-breaks'
import remarkGfm from 'remark-gfm'
import logo from '../logo.png'
import './chat.css'

const CHAT_API_BASE_URL = (import.meta.env.VITE_CHAT_API_BASE_URL || import.meta.env.VITE_API_BASE_URL || '').replace(/\/$/, '')
const markdownPlugins = [remarkGfm, remarkBreaks]
const PROMPT_MOVE_DURATION_MS = 430

function buildApiUrl(path) {
  return `${CHAT_API_BASE_URL}${path}`
}

function createMessage(role, content) {
  return {
    id: `${role}-${Date.now()}-${Math.random().toString(16).slice(2)}`,
    role,
    content,
  }
}

function normalizeMessages(payload) {
  const items = Array.isArray(payload?.message) ? payload.message : []

  return items
    .filter((item) => item && typeof item.content === 'string' && typeof item.role === 'string')
    .map((item) => createMessage(item.role === 'user' ? 'user' : 'assistant', item.content))
}

function getChatTimeValue(chat) {
  if (!chat?.created_time) {
    return Number.NEGATIVE_INFINITY
  }

  const normalizedTime = chat.created_time.includes(' ') ? chat.created_time.replace(' ', 'T') : chat.created_time
  const timestamp = Date.parse(normalizedTime)

  return Number.isNaN(timestamp) ? Number.NEGATIVE_INFINITY : timestamp
}

function sortChatsByCreatedTime(chats) {
  return [...chats].sort((left, right) => {
    const timeDelta = getChatTimeValue(right) - getChatTimeValue(left)

    if (timeDelta !== 0) {
      return timeDelta
    }

    return right.chat_id - left.chat_id
  })
}

function normalizeChats(payload) {
  const items = Array.isArray(payload?.chat) ? payload.chat : []
  const seen = new Set()

  const chats = items
    .filter((item) => item && item.chat_id !== undefined && item.chat_id !== null)
    .map((item) => ({
      chat_id: Number(item.chat_id),
      title: typeof item.title === 'string' && item.title.trim() ? item.title.trim() : 'New Chat',
      created_time: typeof item.created_time === 'string' ? item.created_time : '',
    }))
    .filter((item) => {
      if (Number.isNaN(item.chat_id) || seen.has(item.chat_id)) {
        return false
      }

      seen.add(item.chat_id)
      return true
    })

  return sortChatsByCreatedTime(chats)
}

function getNextChatId(chats) {
  const used = new Set(chats.map((chat) => chat.chat_id))
  let nextId = Math.max(0, ...Array.from(used)) + 1

  while (used.has(nextId)) {
    nextId += 1
  }

  return nextId
}

async function postJson(path, body) {
  const response = await fetch(buildApiUrl(path), {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    credentials: 'include',
    body: body ? JSON.stringify(body) : undefined,
  })

  if (!response.ok) {
    throw new Error(`HTTP ${response.status}`)
  }

  return response.json()
}

function NewChatIcon() {
  return (
    <svg className="new-chat-icon" viewBox="0 0 24 24" aria-hidden="true">
      <path d="M12 3H5a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7" />
      <path d="M18.375 2.625a1 1 0 0 1 3 3l-9.013 9.014a2 2 0 0 1-.853.505l-2.873.84a.5.5 0 0 1-.62-.62l.84-2.873a2 2 0 0 1 .506-.852z" />
    </svg>
  )
}

function SidebarIcon() {
  return (
    <svg className="sidebar-toggle-icon" viewBox="0 0 20 20" aria-hidden="true">
      <rect x="3.5" y="4" width="13" height="12" rx="3" />
      <path d="M8.5 4.5v11" />
    </svg>
  )
}

function UserAvatarIcon() {
  return (
    <svg className="user-avatar" viewBox="0 0 16 16" aria-hidden="true">
      <path d="M11 6a3 3 0 1 1-6 0 3 3 0 0 1 6 0" />
      <path
        fillRule="evenodd"
        d="M0 8a8 8 0 1 1 16 0A8 8 0 0 1 0 8m8-7a7 7 0 0 0-5.468 11.37C3.242 11.226 4.805 10 8 10s4.757 1.225 5.468 2.37A7 7 0 0 0 8 1"
      />
    </svg>
  )
}

export function ChatApp() {
  const [message, setMessage] = useState('')
  const [sidebarOpen, setSidebarOpen] = useState(true)
  const [sidebarAnimating, setSidebarAnimating] = useState(false)
  const [sidebarPhase, setSidebarPhase] = useState('')
  const [tooltip, setTooltip] = useState(null)
  const [chats, setChats] = useState([])
  const [activeChatId, setActiveChatId] = useState(null)
  const [messages, setMessages] = useState([])
  const [isLoadingChats, setIsLoadingChats] = useState(true)
  const [isLoadingHistory, setIsLoadingHistory] = useState(false)
  const [isPreparingHistory, setIsPreparingHistory] = useState(false)
  const [isStreaming, setIsStreaming] = useState(false)
  const [error, setError] = useState('')
  const [tailSpaceHeight, setTailSpaceHeight] = useState(0)
  const activeRequestRef = useRef(0)
  const messageListRef = useRef(null)
  const pendingUserMessageIdRef = useRef(null)
  const hasAnchoredUserMessageRef = useRef(false)
  const activeAssistantMessageIdRef = useRef(null)
  const assistantResizeObserverRef = useRef(null)
  const isAnchoringUserMessageRef = useRef(false)
  const anchorScrollTimeoutRef = useRef(0)
  const historyScrollFrameRef = useRef(0)
  const pendingHistoryScrollRef = useRef(false)
  const lastAssistantHeightRef = useRef(0)
  const lastMessageListScrollTopRef = useRef(0)
  const observedAssistantNodeRef = useRef(null)
  const isTailSpaceSyncingRef = useRef(false)
  const tailSpaceSyncTimeoutRef = useRef(0)
  const tailSpaceHeightRef = useRef(0)
  const bottomRef = useRef(null)
  const username = useMemo(() => window.localStorage.getItem('ttai_user') || 'Tao', [])
  const isChatActive = activeChatId !== null || messages.length > 0 || isLoadingHistory

  const updateTailSpaceHeight = useCallback((nextHeight) => {
    const normalizedHeight = Math.max(0, nextHeight)

    tailSpaceHeightRef.current = normalizedHeight
    setTailSpaceHeight(normalizedHeight)
  }, [])

  const consumeTailSpace = useCallback(
    (distance, isLayoutSync = false) => {
      if (distance <= 0 || tailSpaceHeightRef.current <= 0) {
        return
      }

      if (isLayoutSync) {
        isTailSpaceSyncingRef.current = true
        window.clearTimeout(tailSpaceSyncTimeoutRef.current)
        tailSpaceSyncTimeoutRef.current = window.setTimeout(() => {
          isTailSpaceSyncingRef.current = false
          tailSpaceSyncTimeoutRef.current = 0
        }, 80)
      }

      updateTailSpaceHeight(tailSpaceHeightRef.current - distance)
    },
    [updateTailSpaceHeight],
  )

  const getInitialTailSpaceHeight = useCallback(() => {
    const listHeight = messageListRef.current?.clientHeight || window.innerHeight

    return Math.max(360, Math.round(listHeight * 0.72))
  }, [])

  const scrollMessageListToBottom = useCallback(() => {
    const messageList = messageListRef.current

    if (!messageList) {
      return
    }

    messageList.scrollTop = messageList.scrollHeight
    lastMessageListScrollTopRef.current = messageList.scrollTop
  }, [])

  const queueHistoryScrollToBottom = useCallback(() => {
    window.cancelAnimationFrame(historyScrollFrameRef.current)
    scrollMessageListToBottom()
    historyScrollFrameRef.current = window.requestAnimationFrame(() => {
      scrollMessageListToBottom()
      historyScrollFrameRef.current = window.requestAnimationFrame(scrollMessageListToBottom)
    })
  }, [scrollMessageListToBottom])

  const attachAssistantResizeObserver = useCallback(() => {
    const messageList = messageListRef.current
    const activeAssistantMessageId = activeAssistantMessageIdRef.current
    const assistantNode =
      messageList && activeAssistantMessageId
        ? messageList.querySelector(`[data-message-id="${activeAssistantMessageId}"]`)
        : null

    if (observedAssistantNodeRef.current === assistantNode) {
      return
    }

    assistantResizeObserverRef.current?.disconnect()
    assistantResizeObserverRef.current = null
    observedAssistantNodeRef.current = assistantNode
    lastAssistantHeightRef.current = assistantNode?.getBoundingClientRect().height || 0

    if (!assistantNode || typeof window.ResizeObserver !== 'function') {
      return
    }

    assistantResizeObserverRef.current = new window.ResizeObserver(() => {
      const assistantHeight = assistantNode.getBoundingClientRect().height
      const assistantGrowth = assistantHeight - lastAssistantHeightRef.current

      if (assistantGrowth > 0) {
        consumeTailSpace(assistantGrowth, true)
      }

      lastAssistantHeightRef.current = assistantHeight
    })
    assistantResizeObserverRef.current.observe(assistantNode)
  }, [consumeTailSpace])

  useEffect(() => {
    let ignore = false

    async function loadChats() {
      try {
        const payload = await postJson('/alltitle')

        if (ignore) {
          return
        }

        const nextChats = normalizeChats(payload)
        setChats(nextChats)
      } catch (caughtError) {
        if (!ignore) {
          setError(`无法获取聊天列表：${caughtError instanceof Error ? caughtError.message : '请求失败'}`)
        }
      } finally {
        if (!ignore) {
          setIsLoadingChats(false)
        }
      }
    }

    loadChats()

    return () => {
      ignore = true
    }
  }, [])

  useEffect(
    () => () => {
      assistantResizeObserverRef.current?.disconnect()
      window.clearTimeout(anchorScrollTimeoutRef.current)
      window.clearTimeout(tailSpaceSyncTimeoutRef.current)
      window.cancelAnimationFrame(historyScrollFrameRef.current)
    },
    [],
  )

  useLayoutEffect(() => {
    const pendingUserMessageId = pendingUserMessageIdRef.current
    const messageList = messageListRef.current
    let anchoredPendingMessage = false

    if (pendingUserMessageId && messageList) {
      const messageNode = messageList.querySelector(`[data-message-id="${pendingUserMessageId}"]`)

      if (messageNode) {
        const listRect = messageList.getBoundingClientRect()
        const messageRect = messageNode.getBoundingClientRect()
        const targetOffset = Math.min(Math.max(listRect.height * 0.2, 132), 200)
        const nextScrollTop = messageList.scrollTop + messageRect.top - listRect.top - targetOffset

        isAnchoringUserMessageRef.current = true
        window.clearTimeout(anchorScrollTimeoutRef.current)
        anchorScrollTimeoutRef.current = window.setTimeout(() => {
          isAnchoringUserMessageRef.current = false
          anchorScrollTimeoutRef.current = 0
        }, 700)
        messageList.scrollTo({
          top: Math.max(0, nextScrollTop),
          behavior: 'smooth',
        })
        pendingUserMessageIdRef.current = null
        hasAnchoredUserMessageRef.current = true
        anchoredPendingMessage = true
        lastMessageListScrollTopRef.current = messageList.scrollTop
      }
    }

    attachAssistantResizeObserver()

    if (pendingHistoryScrollRef.current && messageList && messages.length > 0) {
      pendingHistoryScrollRef.current = false
      queueHistoryScrollToBottom()
      return
    }

    if (!anchoredPendingMessage && !isStreaming && !hasAnchoredUserMessageRef.current) {
      scrollMessageListToBottom()
    }
  }, [attachAssistantResizeObserver, isStreaming, messages, queueHistoryScrollToBottom, scrollMessageListToBottom])

  const toggleSidebar = () => {
    setTooltip(null)
    setSidebarPhase(sidebarOpen ? 'closing' : 'opening')
    setSidebarAnimating(true)
    setSidebarOpen((current) => !current)

    window.setTimeout(() => {
      setSidebarAnimating(false)
      setSidebarPhase('')
    }, 360)
  }

  const showSidebarTooltip = (event, label, placement) => {
    if (sidebarAnimating) {
      return
    }

    const rect = event.currentTarget.getBoundingClientRect()

    setTooltip(
      placement === 'bottom'
        ? {
            label,
            left: rect.left + rect.width / 2,
            top: rect.bottom + 8,
            placement,
          }
        : {
            label,
            left: rect.right + 8,
            top: rect.top + rect.height / 2,
            placement,
          },
    )
  }

  const hideSidebarTooltip = () => {
    setTooltip(null)
  }

  const handleMessageListScroll = useCallback(
    (event) => {
      const messageList = event.currentTarget
      const currentScrollTop = messageList.scrollTop

      if (isAnchoringUserMessageRef.current) {
        lastMessageListScrollTopRef.current = currentScrollTop
        return
      }

      if (isTailSpaceSyncingRef.current) {
        lastMessageListScrollTopRef.current = currentScrollTop
        return
      }

      const upwardScrollDistance = lastMessageListScrollTopRef.current - currentScrollTop

      if (upwardScrollDistance > 0) {
        consumeTailSpace(upwardScrollDistance)
      }

      lastMessageListScrollTopRef.current = currentScrollTop
    },
    [consumeTailSpace],
  )

  const handleLogout = async () => {
    setError('')

    try {
      const response = await fetch(buildApiUrl('/logout'), {
        method: 'POST',
        credentials: 'include',
      })

      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`)
      }

      window.localStorage.removeItem('ttai_user')
      window.location.assign('/')
    } catch (caughtError) {
      setError(`退出登录失败：${caughtError instanceof Error ? caughtError.message : '请求失败'}`)
    }
  }

  const handleNewChat = () => {
    activeRequestRef.current += 1
    pendingUserMessageIdRef.current = null
    pendingHistoryScrollRef.current = false
    hasAnchoredUserMessageRef.current = false
    activeAssistantMessageIdRef.current = null
    assistantResizeObserverRef.current?.disconnect()
    assistantResizeObserverRef.current = null
    observedAssistantNodeRef.current = null
    lastAssistantHeightRef.current = 0
    updateTailSpaceHeight(0)
    setActiveChatId(null)
    setMessages([])
    setMessage('')
    setIsPreparingHistory(false)
    setError('')
  }

  const loadHistory = async (chatId) => {
    const requestId = activeRequestRef.current + 1
    const shouldWaitForPromptMove = activeChatId === null && messages.length === 0 && !isLoadingHistory
    const promptMovePromise = shouldWaitForPromptMove
      ? new Promise((resolve) => window.setTimeout(resolve, PROMPT_MOVE_DURATION_MS))
      : Promise.resolve()

    activeRequestRef.current = requestId
    pendingUserMessageIdRef.current = null
    pendingHistoryScrollRef.current = false
    hasAnchoredUserMessageRef.current = false
    activeAssistantMessageIdRef.current = null
    assistantResizeObserverRef.current?.disconnect()
    assistantResizeObserverRef.current = null
    observedAssistantNodeRef.current = null
    lastAssistantHeightRef.current = 0
    updateTailSpaceHeight(0)
    setActiveChatId(chatId)
    setMessages([])
    setError('')
    setIsLoadingHistory(true)
    setIsPreparingHistory(shouldWaitForPromptMove)

    promptMovePromise.then(() => {
      if (activeRequestRef.current === requestId) {
        setIsPreparingHistory(false)
      }
    })

    try {
      const [payload] = await Promise.all([postJson('/history', { chat_id: chatId }), promptMovePromise])

      if (activeRequestRef.current !== requestId) {
        return
      }

      pendingHistoryScrollRef.current = true
      setMessages(normalizeMessages(payload))
    } catch (caughtError) {
      pendingHistoryScrollRef.current = false
      if (activeRequestRef.current === requestId) {
        setError(`无法获取聊天记录：${caughtError instanceof Error ? caughtError.message : '请求失败'}`)
      }
    } finally {
      if (activeRequestRef.current === requestId) {
        setIsPreparingHistory(false)
        setIsLoadingHistory(false)
      }
    }
  }

  const requestTitle = async (chatId, firstMessage) => {
    try {
      const payload = await postJson('/title', {
        chat_id: chatId,
        message: firstMessage,
      })
      const title = typeof payload?.title === 'string' && payload.title.trim() ? payload.title.trim() : 'New Chat'
      const createdTime = typeof payload?.created_time === 'string' ? payload.created_time : ''

      setChats((current) =>
        sortChatsByCreatedTime(
          current.map((chat) =>
            chat.chat_id === chatId ? { ...chat, title, created_time: createdTime || chat.created_time } : chat,
          ),
        ),
      )
    } catch {
      setChats((current) =>
        sortChatsByCreatedTime(current.map((chat) => (chat.chat_id === chatId ? { ...chat, title: 'New Chat' } : chat))),
      )
    }
  }

  const streamAssistantReply = async (chatId, content, assistantMessageId) => {
    const controller = new AbortController()
    let completed = false

    try {
      await fetchEventSource(buildApiUrl('/send'), {
      method: 'POST',
      headers: {
        Accept: 'text/event-stream',
        'Content-Type': 'application/json',
      },
      credentials: 'include',
        signal: controller.signal,
        openWhenHidden: true,
      body: JSON.stringify({
        chat_id: chatId,
        message: content,
      }),
        async onopen(response) {
          if (!response.ok) {
            throw new Error(`HTTP ${response.status}`)
          }
        },
        onmessage(event) {
          const eventText = event.data.trim()

          if (!eventText) {
            return
          }

          if (eventText === '[DONE]') {
            completed = true
            controller.abort()
            return
          }

          try {
            const eventPayload = JSON.parse(eventText)
            const delta = typeof eventPayload.content === 'string' ? eventPayload.content : ''

            if (delta) {
              setMessages((current) =>
                current.map((item) =>
                  item.id === assistantMessageId ? { ...item, content: `${item.content}${delta}` } : item,
                ),
              )
            }
          } catch {
            setMessages((current) =>
              current.map((item) =>
                item.id === assistantMessageId ? { ...item, content: `${item.content}${eventText}` } : item,
              ),
            )
          }
        },
        onerror(error) {
          throw error
        },
      })
    } catch (caughtError) {
      if (completed && controller.signal.aborted) {
        return
      }

      throw caughtError
    }
  }

  const handleSubmit = async (event) => {
    event.preventDefault()

    const content = message.trim()

    if (!content || isStreaming) {
      return
    }

    const isFirstMessage = activeChatId === null
    const chatId = isFirstMessage ? getNextChatId(chats) : activeChatId
    const userMessage = createMessage('user', content)
    const assistantMessage = createMessage('assistant', '')

    setMessage('')
    setError('')
    setIsStreaming(true)
    setActiveChatId(chatId)
    pendingHistoryScrollRef.current = false
    hasAnchoredUserMessageRef.current = false
    activeAssistantMessageIdRef.current = assistantMessage.id
    lastAssistantHeightRef.current = 0
    updateTailSpaceHeight(getInitialTailSpaceHeight())
    pendingUserMessageIdRef.current = userMessage.id
    setMessages((current) => [...current, userMessage, assistantMessage])

    if (isFirstMessage) {
      setChats((current) =>
        sortChatsByCreatedTime([
          {
            chat_id: chatId,
            title: 'New Chat',
            created_time: '',
          },
          ...current,
        ]),
      )
      requestTitle(chatId, content)
    }

    try {
      await streamAssistantReply(chatId, content, assistantMessage.id)
    } catch (caughtError) {
      setError(`发送失败：${caughtError instanceof Error ? caughtError.message : '请求失败'}`)
      setMessages((current) =>
        current.map((item) =>
          item.id === assistantMessage.id && !item.content
            ? { ...item, content: '发送失败，请稍后重试。' }
            : item,
        ),
      )
    } finally {
      activeAssistantMessageIdRef.current = null
      assistantResizeObserverRef.current?.disconnect()
      assistantResizeObserverRef.current = null
      observedAssistantNodeRef.current = null
      setIsStreaming(false)
    }
  }

  return (
    <div
      className={`chat-app ${sidebarOpen ? '' : 'is-sidebar-collapsed'} ${
        sidebarAnimating ? 'is-sidebar-animating' : ''
      } ${sidebarPhase ? `is-sidebar-${sidebarPhase}` : ''}`}
    >
      <aside className="chat-sidebar">
        <div className="sidebar-top">
          <div className="sidebar-brand-row">
            <img className="sidebar-logo" src={logo} alt="TTAI Logo" />

            <button
              className="sidebar-toggle-button sidebar-close-button"
              type="button"
              aria-label="关闭边栏"
              onClick={toggleSidebar}
              onMouseEnter={(event) => showSidebarTooltip(event, '关闭边栏', 'bottom')}
              onMouseLeave={hideSidebarTooltip}
              onFocus={(event) => showSidebarTooltip(event, '关闭边栏', 'bottom')}
              onBlur={hideSidebarTooltip}
            >
              <SidebarIcon />
            </button>

            <button
              className="sidebar-toggle-button sidebar-open-button"
              type="button"
              aria-label="打开边栏"
              onClick={toggleSidebar}
              onMouseEnter={(event) => showSidebarTooltip(event, '打开边栏', 'right')}
              onMouseLeave={hideSidebarTooltip}
              onFocus={(event) => showSidebarTooltip(event, '打开边栏', 'right')}
              onBlur={hideSidebarTooltip}
            >
              <SidebarIcon />
            </button>
          </div>

          <button className="new-chat-button" type="button" onClick={handleNewChat}>
            <NewChatIcon />
            <span className="button-label">新聊天</span>
          </button>

          <nav className="chat-list" aria-label="聊天记录">
            {isLoadingChats && <p className="chat-list-status">加载中...</p>}
            {!isLoadingChats && chats.length === 0 && <p className="chat-list-status">暂无聊天记录</p>}
            {chats.map((chat) => (
              <button
                key={chat.chat_id}
                type="button"
                className={`chat-list-item ${chat.chat_id === activeChatId ? 'is-active' : ''}`}
                onClick={() => loadHistory(chat.chat_id)}
                title={chat.title}
              >
                <span className="chat-list-title">{chat.title}</span>
                {chat.created_time && <span className="chat-list-time">{chat.created_time}</span>}
              </button>
            ))}
          </nav>
        </div>

        <div className="sidebar-account">
          <div className="user-block">
            <UserAvatarIcon />
            <div className="user-copy">
              <p>{username}</p>
              <span>TTAI</span>
            </div>
          </div>

          <button className="logout-button" type="button" onClick={handleLogout}>
            <span className="button-label">退出登录</span>
          </button>
        </div>
      </aside>

      <main className="chat-main">
        <section
          className={`chat-center ${messages.length ? 'has-messages' : ''} ${isChatActive ? 'is-chat-active' : ''}`}
          aria-label="聊天"
        >
          {messages.length === 0 && !isLoadingHistory && (
            <div className="empty-state">
              <h2>准备好了，随时开始</h2>
            </div>
          )}

          {isLoadingHistory && !isPreparingHistory && <p className="history-loading">正在加载聊天记录...</p>}

          {messages.length > 0 && (
            <div
              className="message-list"
              ref={messageListRef}
              onScroll={handleMessageListScroll}
            >
              <div className="chat-column message-column">
                {messages.map((item) => (
                  <article key={item.id} className={`message-bubble is-${item.role}`} data-message-id={item.id}>
                    <div className="message-content">
                      {item.role === 'assistant' ? (
                        <ReactMarkdown remarkPlugins={markdownPlugins}>
                          {item.content || (isStreaming ? '思考中...' : '')}
                        </ReactMarkdown>
                      ) : (
                        item.content
                      )}
                    </div>
                  </article>
                ))}
                <div ref={bottomRef} className="message-tail-space" style={{ height: `${tailSpaceHeight}px` }} aria-hidden="true" />
              </div>
            </div>
          )}

          {error && <p className="chat-error">{error}</p>}

          <form className="chat-column prompt-box" onSubmit={handleSubmit}>
            <input
              type="text"
              value={message}
              onChange={(event) => setMessage(event.target.value)}
              placeholder="有问题，尽管问"
              aria-label="输入消息"
              disabled={isLoadingHistory}
            />
            <button className="send-button" type="submit" aria-label="发送" disabled={!message.trim() || isStreaming}>
              <span aria-hidden="true"></span>
            </button>
          </form>
        </section>
      </main>

      {tooltip && (
        <div
          className={`floating-tooltip is-${tooltip.placement}`}
          style={{
            left: `${tooltip.left}px`,
            top: `${tooltip.top}px`,
          }}
        >
          {tooltip.label}
        </div>
      )}
    </div>
  )
}

createRoot(document.getElementById('chat-root')).render(
  <StrictMode>
    <ChatApp />
  </StrictMode>,
)
