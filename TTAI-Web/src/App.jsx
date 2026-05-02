import { useEffect, useState } from 'react'
import logo from '../logo.png'
import './App.css'

const API_BASE_URL = (import.meta.env.VITE_API_BASE_URL || '').replace(/\/$/, '')

const footerLinks = ['help', 'privacy', 'terms']
const languages = [
  { code: 'zh', label: '简体中文', shortLabel: '中文' },
  { code: 'en', label: 'English', shortLabel: 'EN' },
]

const authStatusText = {
  zh: {
    empty: '请输入账号和密码',
    network: '无法连接服务器',
    badResponse: '返回格式错误',
    http: '请求失败',
    status: {
      1: '用户不存在',
      2: '密码错误',
      3: '格式错误',
      4: '未知错误',
    },
  },
  en: {
    empty: 'Please enter account and password.',
    network: 'Unable to connect to the server.',
    badResponse: 'Invalid response format.',
    http: 'Request failed',
    status: {
      1: 'User not found',
      2: 'Password error',
      3: 'Format error',
      4: 'Unknown error',
    },
  },
}

const copy = {
  zh: {
    title: '登录',
    registerTitle: '注册',
    tagline: 'TTAI 创造无限想象',
    account: '账号',
    password: '密码',
    createAccount: '创建账号',
    back: '返回登录',
    submit: '登录',
    confirm: '确定',
    languageLabel: '语言',
    footerAria: '页脚导航',
    logoAlt: '平台 Logo',
    closeModal: '关闭弹窗',
    github: '联系开发人员 github.com',
    footer: {
      help: '帮助',
      privacy: '隐私权',
      terms: '条款',
    },
    modal: {
      help: {
        title: '帮助',
        paragraphs: [
          '如果你需要找回密码、处理账号无法登录、申请创建账号或反馈页面异常，请联系 TTAI 开发人员。为了便于排查，请尽量说明你的账号信息、遇到问题的时间、操作步骤以及浏览器环境。',
          '当前 TTAI 仍处于开发与试用阶段，部分账号和数据能力可能由开发人员手动维护。开发人员会根据问题类型进行核实，但不承诺所有请求都能即时处理。',
        ],
      },
      privacy: {
        title: '隐私权',
        paragraphs: [
          'TTAI 目前不承诺提供完整的用户隐私保护能力。你在本网站中提交、生成或展示的内容，可能会被用于开发调试、功能验证、问题定位或系统维护。',
          '请不要在 TTAI 中输入身份证件、银行卡、密码、密钥、私人联系方式、商业机密或其他敏感信息。如你对数据使用方式有疑问或异议，请在继续使用前联系开发人员确认。',
        ],
      },
      terms: {
        title: '条款',
        paragraphs: [
          '任何访问或使用 TTAI 网站的用户，均视为已阅读、理解并同意本网站相关说明、隐私权声明以及开发人员后续补充或调整的使用条款。',
          'TTAI 可能随开发进度调整功能、页面、权限、数据保存方式和服务范围。若你不同意这些条款或后续变更，应停止使用本网站，并联系开发人员处理相关问题。',
        ],
      },
    },
  },
  en: {
    title: 'Sign in',
    registerTitle: 'Register',
    tagline: 'TTAI imagines infinite',
    account: 'Account',
    password: 'Password',
    createAccount: 'Create account',
    back: 'Back to sign in',
    submit: 'Sign in',
    confirm: 'Confirm',
    languageLabel: 'Language',
    footerAria: 'Footer navigation',
    logoAlt: 'Platform logo',
    closeModal: 'Close dialog',
    github: 'Contact developers on github.com',
    footer: {
      help: 'Help',
      privacy: 'Privacy',
      terms: 'Terms',
    },
    modal: {
      help: {
        title: 'Help',
        paragraphs: [
          'If you need to recover a password, resolve a sign-in issue, request account creation, or report a page error, please contact the TTAI developers. To help with troubleshooting, include your account information, the time of the issue, the steps you took, and your browser environment when possible.',
          'TTAI is currently in a development and trial stage. Some account and data features may still be maintained manually by developers. Requests will be reviewed based on the issue type, but immediate handling is not guaranteed.',
        ],
      },
      privacy: {
        title: 'Privacy',
        paragraphs: [
          'TTAI does not currently promise complete privacy protection for user data. Content you submit, generate, or display on this website may be used for development debugging, feature validation, issue diagnosis, or system maintenance.',
          'Do not enter identity documents, bank card details, passwords, access keys, private contact information, business secrets, or other sensitive information in TTAI. If you have questions or objections about how data may be used, contact the developers before continuing.',
        ],
      },
      terms: {
        title: 'Terms',
        paragraphs: [
          'By accessing or using the TTAI website, you are considered to have read, understood, and accepted the site notices, privacy statement, and any additional usage terms that developers may supplement or update later.',
          'TTAI may change its features, pages, permissions, data storage behavior, and service scope as development continues. If you do not agree with these terms or future changes, stop using the website and contact the developers for related requests.',
        ],
      },
    },
  },
}

function buildApiUrl(path) {
  return `${API_BASE_URL}${path}`
}

function LoginPage() {
  const [form, setForm] = useState({
    account: '',
    password: '',
  })
  const [language, setLanguage] = useState('zh')
  const [languageMenuOpen, setLanguageMenuOpen] = useState(false)
  const [authMode, setAuthMode] = useState('login')
  const [modal, setModal] = useState({ type: null, closing: false })
  const [authAlert, setAuthAlert] = useState({ content: '', closing: false })

  const text = copy[language]
  const isRegistering = authMode === 'register'
  const activeContent = modal.type ? text.modal[modal.type] : null
  const activeLanguage = languages.find((item) => item.code === language)

  const openModal = (type) => {
    setLanguageMenuOpen(false)
    setModal({ type, closing: false })
  }

  const closeModal = () => {
    setModal((current) => {
      if (!current.type || current.closing) {
        return current
      }

      return { ...current, closing: true }
    })
  }

  const selectLanguage = (code) => {
    setLanguage(code)
    setLanguageMenuOpen(false)
  }

  const showAuthAlert = (content) => {
    setAuthAlert({ content, closing: false })
  }

  const closeAuthAlert = () => {
    setAuthAlert((current) => {
      if (!current.content || current.closing) {
        return current
      }

      return { ...current, closing: true }
    })
  }

  useEffect(() => {
    if (!authAlert.closing) {
      return undefined
    }

    const timer = window.setTimeout(() => {
      setAuthAlert({ content: '', closing: false })
    }, 180)

    return () => window.clearTimeout(timer)
  }, [authAlert.closing])

  useEffect(() => {
    if (!modal.closing) {
      return undefined
    }

    const timer = window.setTimeout(() => {
      setModal({ type: null, closing: false })
    }, 220)

    return () => window.clearTimeout(timer)
  }, [modal.closing])

  useEffect(() => {
    const handleKeyDown = (event) => {
      if (event.key === 'Escape') {
        if (authAlert.content) {
          closeAuthAlert()
          return
        }

        if (modal.type) {
          closeModal()
          return
        }

        setLanguageMenuOpen(false)
      }
    }

    window.addEventListener('keydown', handleKeyDown)
    return () => window.removeEventListener('keydown', handleKeyDown)
  }, [authAlert.content, modal.type])

  const handleChange = ({ target: { name, value } }) => {
    setForm((current) => ({
      ...current,
      [name]: value,
    }))
  }

  const handleSubmit = async (event) => {
    event.preventDefault()

    if (!form.account.trim() || !form.password) {
      showAuthAlert(authStatusText[language].empty)
      return
    }

    try {
      const response = await fetch(buildApiUrl(isRegistering ? '/register' : '/login'), {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        credentials: 'include',
        body: JSON.stringify({
          user: form.account.trim(),
          password: form.password,
        }),
      })

      if (!response.ok) {
        showAuthAlert(`${authStatusText[language].http}: HTTP ${response.status}`)
        return
      }

      const result = await response.json().catch(() => null)

      if (!result || typeof result.status !== 'number') {
        showAuthAlert(authStatusText[language].badResponse)
        return
      }

      if (result.status === 0) {
        window.localStorage.setItem('ttai_user', form.account.trim())
        window.location.assign('/chat/')
        return
      }

      showAuthAlert(authStatusText[language].status[result.status] || authStatusText[language].status[4])
    } catch (error) {
      showAuthAlert(error instanceof Error ? error.message : authStatusText[language].network)
    }
  }

  const toggleAuthMode = () => {
    setAuthMode((current) => (current === 'login' ? 'register' : 'login'))
  }

  return (
    <div className="login-page" lang={language === 'zh' ? 'zh-CN' : 'en'}>
      <main className="login-shell">
        <section className="login-card">
          <div className="login-intro">
            <img className="brand-logo" src={logo} alt={text.logoAlt} />

            <div className="brand-copy">
              <h1>{isRegistering ? text.registerTitle : text.title}</h1>
              <p>{text.tagline}</p>
            </div>
          </div>

          <div className="login-form-panel">
            <form className="login-form" onSubmit={handleSubmit}>
              <label className={`form-field ${form.account ? 'has-value' : ''}`}>
                <input
                  type="text"
                  name="account"
                  value={form.account}
                  onChange={handleChange}
                  placeholder=" "
                  autoComplete="username"
                  aria-label={text.account}
                />
                <span className="form-field-label">{text.account}</span>
              </label>

              <label className={`form-field ${form.password ? 'has-value' : ''}`}>
                <input
                  type="password"
                  name="password"
                  value={form.password}
                  onChange={handleChange}
                  placeholder=" "
                  autoComplete={isRegistering ? 'new-password' : 'current-password'}
                  aria-label={text.password}
                />
                <span className="form-field-label">{text.password}</span>
              </label>

              <div className="action-row">
                <button type="button" className="link-button" onClick={toggleAuthMode}>
                  {isRegistering ? text.back : text.createAccount}
                </button>

                <button type="submit" className="primary-button">
                  {isRegistering ? text.confirm : text.submit}
                </button>
              </div>
            </form>
          </div>
        </section>

        <footer className="login-footer">
          <div className={`language-selector ${languageMenuOpen ? 'is-open' : ''}`}>
            <button
              type="button"
              className="language-button"
              aria-haspopup="listbox"
              aria-expanded={languageMenuOpen}
              onClick={() => setLanguageMenuOpen((current) => !current)}
            >
              <span className="language-current">
                <span>{activeLanguage.label}</span>
              </span>
              <span className="language-caret" aria-hidden="true"></span>
            </button>

            {languageMenuOpen && (
              <div className="language-menu" role="listbox" aria-label={text.languageLabel}>
                {languages.map((item) => (
                  <button
                    key={item.code}
                    type="button"
                    className={`language-option ${item.code === language ? 'is-selected' : ''}`}
                    role="option"
                    aria-selected={item.code === language}
                    onClick={() => selectLanguage(item.code)}
                  >
                    <span>{item.label}</span>
                    <span className="language-option-short">{item.shortLabel}</span>
                  </button>
                ))}
              </div>
            )}
          </div>

          <nav className="footer-links" aria-label={text.footerAria}>
            {footerLinks.map((item) => (
              <button key={item} type="button" onClick={() => openModal(item)}>
                {text.footer[item]}
              </button>
            ))}
          </nav>
        </footer>
      </main>

      {authAlert.content && (
        <div
          className={`auth-alert-backdrop ${authAlert.closing ? 'is-closing' : ''}`}
          onMouseDown={closeAuthAlert}
        >
          <section
            className="auth-alert"
            role="alertdialog"
            aria-modal="true"
            aria-labelledby="auth-alert-title"
            aria-describedby="auth-alert-message"
            onMouseDown={(event) => event.stopPropagation()}
          >
            <h2 id="auth-alert-title">TTAI</h2>
            <p id="auth-alert-message">{authAlert.content}</p>
            <button type="button" className="auth-alert-button" onClick={closeAuthAlert}>
              {text.confirm}
            </button>
          </section>
        </div>
      )}

      {activeContent && (
        <div
          className={`modal-backdrop ${modal.closing ? 'is-closing' : ''}`}
          onMouseDown={closeModal}
        >
          <section
            className="info-modal"
            role="dialog"
            aria-modal="true"
            aria-labelledby="info-modal-title"
            onMouseDown={(event) => event.stopPropagation()}
          >
            <header className="info-modal-header">
              <div>
                <p className="info-modal-kicker">TTAI</p>
                <h2 id="info-modal-title">{activeContent.title}</h2>
              </div>
              <button
                type="button"
                className="modal-close-button"
                aria-label={text.closeModal}
                onClick={closeModal}
              >
                <span>x</span>
              </button>
            </header>

            <div className="info-modal-body">
              {activeContent.paragraphs.map((paragraph) => (
                <p key={paragraph}>{paragraph}</p>
              ))}
              <a className="modal-github-link" href="https://github.com/TaotaoLyu" target="_blank" rel="noreferrer">
                {text.github}
              </a>
            </div>
          </section>
        </div>
      )}
    </div>
  )
}

function App() {
  return <LoginPage />
}

export default App
