#ifndef VIKING_DATA_DOWNLOADER_H
#define VIKING_DATA_DOWNLOADER_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QSharedPointer>

class Structure;

//! Downloads and parses JSON data from viking database
/*!
 * The Downloader downloads and parses JSON data from the viking database
 */
class Downloader : public QObject
{
  Q_OBJECT

public:
  Downloader();
  ~Downloader();

  QSharedPointer<Structure> download_structure( int id );

  QList<QVariant> get_location_list();
  QList<QVariant> get_link_list();

public Q_SLOTS:

  void http_finished();
  void http_ready_read();
  void update_data_read_progress( qint64 bytes_read, qint64 total_bytes );

private:

  QList<QVariant> download_json(QString url_string, QString file_prefix);

  QString download_url(QString url_string);

  QNetworkAccessManager qnam_;
  QNetworkReply* reply_;

  bool http_request_aborted;
};

#endif /* VIKING_DATA_DOWNLOADER_H */
