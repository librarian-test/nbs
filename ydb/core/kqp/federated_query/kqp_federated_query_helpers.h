#pragma once

#include <library/cpp/actors/core/actorsystem.h>

#include <ydb/core/base/appdata.h>
#include <ydb/library/yql/providers/common/db_id_async_resolver/db_async_resolver.h>
#include <ydb/library/yql/providers/common/db_id_async_resolver/mdb_endpoint_generator.h>
#include <ydb/library/yql/providers/common/http_gateway/yql_http_gateway.h>
#include <ydb/library/yql/providers/common/token_accessor/client/factory.h>
#include <ydb/library/yql/providers/generic/connector/libcpp/client.h>

namespace NKikimr::NKqp {

    struct TKqpFederatedQuerySetup {
        NYql::IHTTPGateway::TPtr HttpGateway;
        NYql::NConnector::IClient::TPtr ConnectorClient;
        NYql::ISecuredServiceAccountCredentialsFactory::TPtr CredentialsFactory;
        NYql::IDatabaseAsyncResolver::TPtr DatabaseAsyncResolver;
        NYql::TS3GatewayConfig S3GatewayConfig;
    };

    struct IKqpFederatedQuerySetupFactory {
        using TPtr = std::shared_ptr<IKqpFederatedQuerySetupFactory>;
        virtual std::optional<TKqpFederatedQuerySetup> Make(NActors::TActorSystem* actorSystem) = 0;
        virtual ~IKqpFederatedQuerySetupFactory() = default;
    };

    struct TKqpFederatedQuerySetupFactoryNoop: public IKqpFederatedQuerySetupFactory {
        std::optional<TKqpFederatedQuerySetup> Make(NActors::TActorSystem*) override {
            return std::nullopt;
        }
    };

    struct TKqpFederatedQuerySetupFactoryDefault: public IKqpFederatedQuerySetupFactory {
        TKqpFederatedQuerySetupFactoryDefault(){};

        TKqpFederatedQuerySetupFactoryDefault(
            NActors::TActorSystemSetup* setup,
            const NKikimr::TAppData* appData,
            const NKikimrConfig::TAppConfig& appConfig);

        std::optional<TKqpFederatedQuerySetup> Make(NActors::TActorSystem* actorSystem) override;

    private:
        NYql::THttpGatewayConfig HttpGatewayConfig;
        NYql::IHTTPGateway::TPtr HttpGateway;
        NYql::TS3GatewayConfig S3GatewayConfig;
        NYql::ISecuredServiceAccountCredentialsFactory::TPtr CredentialsFactory;
        NYql::NConnector::IClient::TPtr ConnectorClient;
        std::optional<NActors::TActorId> DatabaseResolverActorId;
        NYql::IMdbEndpointGenerator::TPtr MdbEndpointGenerator;
        std::optional<TString> MdbGateway;
    };

    struct TKqpFederatedQuerySetupFactoryMock: public IKqpFederatedQuerySetupFactory {
        TKqpFederatedQuerySetupFactoryMock() = delete;

        TKqpFederatedQuerySetupFactoryMock(
            NYql::IHTTPGateway::TPtr httpGateway,
            NYql::NConnector::IClient::TPtr connectorClient,
            NYql::ISecuredServiceAccountCredentialsFactory::TPtr credentialsFactory,
            NYql::IDatabaseAsyncResolver::TPtr databaseAsyncResolver,
            const NYql::TS3GatewayConfig& s3GatewayConfig)
            : HttpGateway(httpGateway)
            , ConnectorClient(connectorClient)
            , CredentialsFactory(credentialsFactory)
            , DatabaseAsyncResolver(databaseAsyncResolver)
            , S3GatewayConfig(s3GatewayConfig)
        {
        }

        std::optional<TKqpFederatedQuerySetup> Make(NActors::TActorSystem*) override {
            return TKqpFederatedQuerySetup{
                HttpGateway, ConnectorClient, CredentialsFactory, DatabaseAsyncResolver, S3GatewayConfig};
        }

    private:
        NYql::IHTTPGateway::TPtr HttpGateway;
        NYql::NConnector::IClient::TPtr ConnectorClient;
        NYql::ISecuredServiceAccountCredentialsFactory::TPtr CredentialsFactory;
        NYql::IDatabaseAsyncResolver::TPtr DatabaseAsyncResolver;
        NYql::TS3GatewayConfig S3GatewayConfig;
    };

    IKqpFederatedQuerySetupFactory::TPtr MakeKqpFederatedQuerySetupFactory(
        NActors::TActorSystemSetup* setup,
        const NKikimr::TAppData* appData,
        const NKikimrConfig::TAppConfig& config);
}
