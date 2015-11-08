#include "ui/mainwindow.h"

#include <sstream>

#include <QDateTime>
#include <QMessageBox>
#include <QInputDialog>
#include <QCloseEvent>

#include <windows.h>
#include <lm.h>

#include "ui/wlan_host_table_model.h"
#include "wlan/wlan_host.h"
#include "sharing/sharing_service.h"
#include "network/ics_service.h"
#include "network/ics_connection.h"
#include "app_settings.h"

namespace ui {

using ui::WlanHostTableModel;
using wlan::HostedWlan;
using wlan::WlanHost;
using sharing::SharingService;
using sharing::SharedResource;
using network::IcsService;
using network::IcsConnection;

MainWindow::MainWindow(std::shared_ptr<HostedWlan> wlan, QWidget *parent)
    : QMainWindow(parent), wlan_(wlan), wlan_controller_(wlan),
      ico_wifi_on_(":/app_resources/images/wifi_on.ico"),
      ico_wifi_off_(":/app_resources/images/wifi_off.ico")
{
    ui_.setupUi(this);
    this->setWindowTitle(AppSettings::APP_NAME);

    this->setupExtraUi();
    this->updateSharingView();
    this->keepViewSync();
    this->connectViewSignals();

    this->wlan_->updateProperties();
    this->ui_.txtSSID->setText(QString::fromStdString(this->wlan_->ssid()));
    this->ui_.txtKey->setText(QString::fromStdString(this->wlan_->secondaryKey()));
    this->wlan_->updateStatus();
    this->reloadPeers();
    this->updateWlanView();
    this->updatePeersView();

    connect(this->ui_.actionToggle, &QAction::triggered, this->wlan_.get(), &HostedWlan::toggle);
    IcsService::instance()->reloadConnections();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange && this->isMinimized())
    {
        this->hide();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    this->showMinimized();
    this->hide();
    event->ignore();
}

void MainWindow::on_btnSwitch_clicked()
{
    auto err = this->wlan_->toggle();
    if (err != wlan_hosted_network_reason_success)
    {
        auto msg = QString::fromLocal8Bit("切换失败 -_-# ：") + QString::fromStdWString(HostedWlan::getFailReason(err));
        log(msg);
    }
    else
    {
        log(QString::fromLocal8Bit("OK :-)"));
    }
}

void MainWindow::on_btnReloadConnections_clicked()
{
    this->ui_.btnReloadConnections->setDisabled(true);
    IcsService::instance()->reloadConnections();
}

void MainWindow::on_btnApplySettings_clicked()
{
    this->ui_.btnApplySettings->setEnabled(false);
    this->setCursor(Qt::WaitCursor);

    auto ssid = this->ui_.txtSSID->text().toStdString();
    auto key = this->ui_.txtKey->text().toStdString();
    std::string sharing_conn;
    if (this->ui_.grpEnableIcs->isChecked())
    {
        sharing_conn = this->ui_.cmbConnections->currentIndex() > -1 ? this->ui_.cmbConnections->currentData().value<QString>().toStdString() : "";
    }

    auto rslt = this->wlan_controller_.applyConfigs(ssid, key, sharing_conn);
    if (!rslt.empty())
        QMessageBox::critical(this, "error", QString::fromStdWString(rslt));
    else
        log(QString::fromLocal8Bit("OK :-)"));

    this->setCursor(Qt::ArrowCursor);
    this->ui_.btnApplySettings->setEnabled(true);
}

void MainWindow::on_sharing_action()
{
    auto action = dynamic_cast<QAction *>(sender())->data().toString();
    if (action == "add")
    {
        auto input = QInputDialog::getText(this, QString::fromLocal8Bit("输入路径"), "");
        if (!input.trimmed().isEmpty())
        {
            SharedResource resource;
            resource.path = input.toStdWString();
            resource.name = input.toStdWString();
            auto ret = SharingService::instance()->add(resource);
            if (ret != NERR_Success)
            {
                QMessageBox::critical(this, "error", SharingService::getErrorString(ret));
            }
        }
    }
    else if (action == "remove")
    {
        auto selected = this->ui_.tblvwShares->selectionModel()->selectedIndexes();
        for (const auto &i : selected)
        {
            if (i.column() == 0)
            {
                auto data = this->shared_folders_model_.data(i);
                SharingService::instance()->remove(data.toString().toStdWString());
            }
        }
    }
    else if (action == "remove_all")
    {
        for (const auto &i : SharingService::instance()->all())
        {
            if (i.type == STYPE_DISKTREE)
            {
                SharingService::instance()->remove(i.name);
            }
        }
    }
    else if (action == "reload")
    {
        this->updateSharingView();
    }
}

void MainWindow::setupExtraUi()
{
    /// @{共享管理菜单
    this->ctx_menu_sharing_.setFont(this->ui_.menuBar->font());
    this->ctx_menu_sharing_.addAction(QString::fromLocal8Bit("添加"), this, SLOT(on_sharing_action()))->setData("add");
    this->ctx_menu_sharing_.addAction(QString::fromLocal8Bit("移除"), this, SLOT(on_sharing_action()))->setData("remove");
    this->ctx_menu_sharing_.addAction(QString::fromLocal8Bit("移除全部"), this, SLOT(on_sharing_action()))->setData("remove_all");
    this->ctx_menu_sharing_.addAction(QString::fromLocal8Bit("刷新"), this, SLOT(on_sharing_action()))->setData("reload");
    /// @}

    /// @{托盘
    this->ctx_menu_tray_.setFont(this->ui_.menuBar->font());
    this->ctx_menu_tray_.addAction(this->ui_.actionToggle);
    this->ctx_menu_tray_.addAction(this->ui_.actionSettings);
    this->ctx_menu_tray_.addAction(this->ui_.actionExit);
    this->tray_.setContextMenu(&this->ctx_menu_tray_);
    this->tray_.show();
    /// @}

    this->ctx_menu_peers_.setFont(this->ui_.menuBar->font());
    this->ctx_menu_peers_.addAction(QString::fromLocal8Bit("刷新"), this, SLOT(reloadPeers()));
}

void MainWindow::connectViewSignals()
{
    connect(this->ui_.actionExit, &QAction::triggered, &QApplication::quit);
    connect(this->ui_.actionSettings, &QAction::triggered, [this](bool)
    {
        this->dlg_settings_.reload();
        if (this->isHidden())
        {
            this->show();
            this->setWindowState(Qt::WindowState::WindowNoState | Qt::WindowState::WindowActive);
        }
        this->dlg_settings_.show();
    });
    connect(this->ui_.actionAbout, &QAction::triggered, [this](bool)
    {
        const auto url = "https://github.com/rubiest/QSoftAP";
        QMessageBox::about(this, QString::fromLocal8Bit("关于"), QString::fromLocal8Bit("源码：<a href='%1'>%1</a>").arg(url));
    });
    connect(&this->tray_, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason)
    {
        if (reason == QSystemTrayIcon::Trigger)
        {
            if (this->isHidden())
            {
                this->show();
                this->setWindowState(Qt::WindowState::WindowNoState | Qt::WindowState::WindowActive);
            }
        }
    });
}

void MainWindow::keepViewSync()
{
    connect(SharingService::instance(), &SharingService::sharesChanged, this, &MainWindow::updateSharingView);

    connect(IcsService::instance(), &IcsService::reloadFinished, [this](IcsService & sender)
    {
        this->ui_.cmbConnections->clear();
        for (auto &i : sender.connections)
        {
            if (this->wlan_->adapterID() != i.guid())
            {
                this->ui_.cmbConnections->addItem(
                    QString::fromStdWString(i.name()),
                    QString::fromStdString(i.guid()));

                if (i.sharingEnabled() && i.isPublic())
                    this->ui_.cmbConnections->setCurrentIndex(this->ui_.cmbConnections->findData(i.guid().c_str()));
            }
        }
        this->ui_.btnReloadConnections->setDisabled(false);
    });

    this->ui_.tblvwHosts->setModel(&this->host_table_model_);
    connect(this->wlan_.get(), &HostedWlan::peerJoined, &this->host_table_model_, &WlanHostTableModel::onPeerJoined);
    connect(this->wlan_.get(), &HostedWlan::peerJoined, this, [this](std::shared_ptr<HostedWlan> sender, std::shared_ptr<WlanHost> peer) {
        this->updatePeersView();
        std::ostringstream msg;
        msg << peer->mac_address_hex << " 加入连接";
        this->tray_.showMessage("", QString::fromLocal8Bit(msg.str().c_str()));
    }, Qt::QueuedConnection);

    connect(this->wlan_.get(), &HostedWlan::peerLeaved, &this->host_table_model_, &WlanHostTableModel::onPeerLeaved);
    connect(this->wlan_.get(), &HostedWlan::peerLeaved, this, [this](std::shared_ptr<HostedWlan> sender, std::string mac) {
        this->updatePeersView();
        std::ostringstream msg;
        msg << mac << " 断开连接";
        this->tray_.showMessage("", QString::fromLocal8Bit(msg.str().c_str()));
    }, Qt::QueuedConnection);

    connect(this->wlan_.get(), &HostedWlan::stateChanged, this, [this](std::shared_ptr<HostedWlan>)
    {
        this->updateWlanView();
        this->reloadPeers();
        this->updatePeersView();
    }, Qt::QueuedConnection);
}

void MainWindow::updateWlanView()
{
    switch (this->wlan_->state())
    {
    case wlan_hosted_network_unavailable:
    {
        this->ui_.btnSwitch->setChecked(false);

        this->tray_.setIcon(this->ico_wifi_off_);
        this->setWindowIcon(this->ico_wifi_off_);

        this->ui_.actionToggle->setText(QString::fromLocal8Bit("开启"));

        auto ret = this->wlan_->setEnabled(TRUE);
        if (ret != wlan_hosted_network_reason_success)
            this->log(QString::fromStdWString(this->wlan_->getFailReason(ret)));
    }
    break;
    case wlan_hosted_network_active:
        this->ui_.btnSwitch->setChecked(true);

        this->tray_.setIcon(this->ico_wifi_on_);
        this->setWindowIcon(this->ico_wifi_on_);

        this->ui_.actionToggle->setText(QString::fromLocal8Bit("关闭"));
        break;
    case wlan_hosted_network_idle:
        this->ui_.btnSwitch->setChecked(false);

        this->tray_.setIcon(this->ico_wifi_off_);
        this->setWindowIcon(this->ico_wifi_off_);

        this->ui_.actionToggle->setText(QString::fromLocal8Bit("开启"));
    default:
        break;
    }
    this->ui_.btnSwitch->setEnabled(this->wlan_->state() != wlan_hosted_network_unavailable);
    IcsConnection conn;
    if (IcsService::instance()->findById(this->wlan_->adapterID(), conn))
    {
        this->ui_.grpEnableIcs->setChecked(conn.sharingEnabled() && conn.isPrivate());
    }
}

void MainWindow::updatePeersView()
{
    this->ui_.grpHosts->setTitle(QString::fromLocal8Bit("主机列表（%1）").arg(this->wlan_->peers.size()));
    this->ui_.tblvwHosts->setColumnHidden(1, true);
}

void MainWindow::reloadPeers()
{
    this->host_table_model_.removeRows(0, this->host_table_model_.peers.size());
    for (auto i : this->wlan_->peers)
        this->host_table_model_.onPeerJoined(this->wlan_, i.second);
}

void MainWindow::updateSharingView()
{
    this->shared_folders_model_.setShares(SharingService::instance()->all());
    this->ui_.tblvwShares->setModel(nullptr);
    this->ui_.tblvwShares->setModel(&this->shared_folders_model_);
    this->ui_.tblvwShares->setColumnHidden(0, true);
}

void MainWindow::on_tblvwShares_customContextMenuRequested(const QPoint &loc)
{
    auto global_pos = this->ui_.tblvwShares->mapToGlobal(loc);
    this->ctx_menu_sharing_.exec(global_pos);
}

void MainWindow::on_tblvwHosts_customContextMenuRequested(const QPoint &loc)
{
    auto global_pos = this->ui_.tblvwHosts->mapToGlobal(loc);
    this->ctx_menu_peers_.exec(global_pos);
}

void MainWindow::log(const QString &msg)
{
    this->ui_.statusBar->showMessage(QString("%1 : %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz")).arg(msg));
}

}
